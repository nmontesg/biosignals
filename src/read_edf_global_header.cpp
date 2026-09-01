#include "duckdb.hpp" // IWYU pragma: keep
#include "biosignals_edf_parser.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "read_edf_global_header.hpp"

namespace duckdb {

unique_ptr<FunctionData> ReadEDFGlobalHeaderFuncBind(ClientContext &context, TableFunctionBindInput &input,
                                                     vector<LogicalType> &return_types, vector<string> &names) {
	if (input.inputs.empty()) {
		throw InvalidInputException("read_edf_global_header requires at least one argument.");
	}

	auto &path_param = input.inputs[0];
	string glob_pattern = StringValue::Get(path_param);
	auto &fs = FileSystem::GetFileSystem(context);
	auto file_list = fs.Glob(glob_pattern);
	if (file_list.empty()) {
		throw IOException("No files found matching the provided pattern.");
	}

	auto result = make_uniq<ReadEDFGlobalHeaderBindData>();
	result->files = std::move(file_list);

	names.push_back("filename");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("format");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("version");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("patient_id");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("recording_id");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("start_datetime");
	return_types.push_back(LogicalType::TIMESTAMP_S);

	names.push_back("num_records");
	return_types.push_back(LogicalType::INTEGER);

	names.push_back("duration");
	return_types.push_back(LogicalType::FLOAT);

	names.push_back("num_signals");
	return_types.push_back(LogicalType::INTEGER);

	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> ReadEDFGlobalHeaderGlobalInit(ClientContext &context,
                                                                   TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<ReadEDFGlobalHeaderBindData>();
	return make_uniq<ReadEDFGlobalHeaderGlobalState>(bind_data.files.size());
}

unique_ptr<LocalTableFunctionState> ReadEDFGlobalHeaderLocalInit(ExecutionContext &context,
                                                                 TableFunctionInitInput &input,
                                                                 GlobalTableFunctionState *global_state_p) {
	return make_uniq<ReadEDFGlobalHeaderLocalState>();
}

void SetEmptyStringFieldToNull(const string &hdr_field, Vector &vector, string_t *data, const idx_t idx) {
	if (hdr_field.empty()) {
		vector.SetValue(idx, Value(nullptr));
	} else {
		data[idx] = StringVector::AddString(vector, hdr_field);
	}
}

void ReadEDFGlobalHeaderFunc(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &global_state = data.global_state->Cast<ReadEDFGlobalHeaderGlobalState>();
	auto &bind_data = data.bind_data->Cast<ReadEDFGlobalHeaderBindData>();

	string dicom_logtype = "dicom";

	idx_t start_idx, end_idx;
	if (!global_state.GetWorkItem(start_idx, end_idx)) {
		output.SetCardinality(0);
		return;
	}

	idx_t count = end_idx - start_idx;
	auto &filename_vector = output.data[0];
	auto &format_vector = output.data[1];
	auto &version_vector = output.data[2];
	auto &patient_id_vector = output.data[3];
	auto &recording_id_vector = output.data[4];
	auto &start_datetime_vector = output.data[5];
	auto &num_records_vector = output.data[6];
	auto &duration_vector = output.data[7];
	auto &num_signals_vector = output.data[8];

	auto filename_data = FlatVector::GetData<string_t>(filename_vector);
	auto format_data = FlatVector::GetData<string_t>(format_vector);
	auto version_data = FlatVector::GetData<string_t>(version_vector);
	auto patient_id_data = FlatVector::GetData<string_t>(patient_id_vector);
	auto recording_id_data = FlatVector::GetData<string_t>(recording_id_vector);

	auto &fs = FileSystem::GetFileSystem(context);

	idx_t actual_count = 0;
	for (idx_t i = 0; i < count; i++) {
		idx_t file_index = start_idx + i;

		// filename column
		auto filename = bind_data.files[file_index].path;
		filename_data[i] = StringVector::AddString(filename_vector, filename);

		auto file_handle = fs.OpenFile(filename, FileOpenFlags::FILE_FLAGS_READ);
		auto global_header = BiosignalsEDFParser::ParseGlobalHeader(*file_handle);

		// format column
		string format_value =
		    (global_header.is_bdf ? string("BDF") : string("EDF")) + (global_header.is_plus ? string("+") : string(""));
		if (global_header.is_plus) {
			format_value += global_header.is_discontinuous ? string("D") : string("C");
		}
		format_data[i] = StringVector::AddString(format_vector, format_value);

		// version, patient_id, record_id columns
		SetEmptyStringFieldToNull(global_header.version, version_vector, version_data, i);
		SetEmptyStringFieldToNull(global_header.patient_id, patient_id_vector, patient_id_data, i);
		SetEmptyStringFieldToNull(global_header.recording_id, recording_id_vector, recording_id_data, i);

		// num_records, duration, num_signals columns
		num_records_vector.SetValue(i, Value::INTEGER(global_header.num_records));
		duration_vector.SetValue(i, Value::FLOAT(global_header.duration_record));
		num_signals_vector.SetValue(i, Value::INTEGER(global_header.num_signals));

		// start_datetime column
		timestamp_t start_timestamp = Timestamp::FromString(global_header.GetFormattedStartTimestamp(), false);
		start_datetime_vector.SetValue(i, Value::TIMESTAMP(start_timestamp));

		actual_count += 1;
	}

	output.SetCardinality(actual_count);
}

unique_ptr<NodeStatistics> ReadEDFGlobalHeaderCardinality(ClientContext &context, const FunctionData *bind_data_p) {
	auto &bind_data = bind_data_p->Cast<ReadEDFGlobalHeaderBindData>();
	return make_uniq<NodeStatistics>(static_cast<idx_t>(bind_data.files.size()));
}

double ReadEDFGlobalHeaderProgress(ClientContext &context, const FunctionData *bind_data_p,
                                   const GlobalTableFunctionState *global_state_p) {
	auto &bind_data = bind_data_p->Cast<ReadEDFGlobalHeaderBindData>();
	auto &global_state = global_state_p->Cast<ReadEDFGlobalHeaderGlobalState>();

	if (bind_data.files.empty()) {
		return 100.0;
	}
	idx_t files_processed = bind_data.files.size() - global_state.num_files_left_to_read;
	return 100.0 * static_cast<double>(files_processed) / static_cast<double>(bind_data.files.size());
}

void RegisterReadEDFGlobalHeader(ExtensionLoader &loader) {
	TableFunction read_edf_global_header_func("read_edf_global_header", {LogicalType::VARCHAR}, ReadEDFGlobalHeaderFunc,
	                                          ReadEDFGlobalHeaderFuncBind, ReadEDFGlobalHeaderGlobalInit,
	                                          ReadEDFGlobalHeaderLocalInit);

	CreateTableFunctionInfo read_edf_global_header_info(read_edf_global_header_func);
	FunctionDescription read_edf_global_header_desc;

	read_edf_global_header_desc.parameter_types = {LogicalType::VARCHAR, LogicalType::BOOLEAN};
	read_edf_global_header_desc.parameter_names = {"path"};
	read_edf_global_header_desc.description = "Read global header information from EDF(+) / BDF(+) files into DuckDB.";
	read_edf_global_header_desc.examples = {"FROM read_edf_global_header('/path/to/recording.edf');",
	                                        "FROM read_edf_global_header('/path/to/all_recordings/*');"};
	read_edf_global_header_desc.categories = {"medical"};
	read_edf_global_header_info.descriptions.push_back(read_edf_global_header_desc);

	read_edf_global_header_func.cardinality = ReadEDFGlobalHeaderCardinality;
	read_edf_global_header_func.table_scan_progress = ReadEDFGlobalHeaderProgress;

	loader.RegisterFunction(read_edf_global_header_info);
}

} // namespace duckdb
