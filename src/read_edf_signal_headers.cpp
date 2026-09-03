#include "duckdb.hpp" // IWYU pragma: keep
#include "biosignals_edf_parser.hpp"
#include "duckdb/common/types/value.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "read_edf_global_header.hpp"
#include "read_edf_signal_headers.hpp"

namespace duckdb {

unique_ptr<FunctionData> ReadEDFSignalHeadersFuncBind(ClientContext &context, TableFunctionBindInput &input,
                                                      vector<LogicalType> &return_types, vector<string> &names) {
	if (input.inputs.empty()) {
		throw InvalidInputException("read_edf_signal_headers requires at least one argument.");
	}

	auto &path_param = input.inputs[0];
	string glob_pattern = StringValue::Get(path_param);
	auto &fs = FileSystem::GetFileSystem(context);
	auto file_list = fs.Glob(glob_pattern);
	if (file_list.empty()) {
		throw IOException("No files found matching the provided pattern.");
	}

	auto result = make_uniq<ReadEDFSignalHeadersBindData>();
	result->files = std::move(file_list);

	names.push_back("filename");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("signal_id");
	return_types.push_back(LogicalType::UINTEGER);

	names.push_back("label");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("transducer_type");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("physical_dim");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("physical_min");
	return_types.push_back(LogicalType::FLOAT);

	names.push_back("physical_max");
	return_types.push_back(LogicalType::FLOAT);

	names.push_back("digital_min");
	return_types.push_back(LogicalType::INTEGER);

	names.push_back("digital_max");
	return_types.push_back(LogicalType::INTEGER);

	names.push_back("prefiltering");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("samples_per_record");
	return_types.push_back(LogicalType::UINTEGER);

	names.push_back("is_annotation");
	return_types.push_back(LogicalType::BOOLEAN);

	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> ReadEDFSignalHeadersGlobalInit(ClientContext &context,
                                                                    TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<ReadEDFSignalHeadersBindData>();
	return make_uniq<ReadEDFSignalHeadersGlobalState>(bind_data.files.size());
}

unique_ptr<LocalTableFunctionState> ReadEDFSignalHeadersLocalInit(ExecutionContext &context,
                                                                  TableFunctionInitInput &input,
                                                                  GlobalTableFunctionState *global_state_p) {
	return make_uniq<ReadEDFSignalHeadersLocalState>();
}

void ReadEDFSignalHeadersFunc(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &global_state = data.global_state->Cast<ReadEDFSignalHeadersGlobalState>();
	auto &bind_data = data.bind_data->Cast<ReadEDFSignalHeadersBindData>();

	idx_t start_idx, end_idx;
	if (!global_state.GetWorkItem(start_idx, end_idx)) {
		output.SetCardinality(0);
		return;
	}

	idx_t count = end_idx - start_idx;

	auto &filename_vector = output.data[0];
	auto &signal_id_vector = output.data[1];
	auto &label_vector = output.data[2];
	auto &transducer_type_vector = output.data[3];
	auto &physical_dim_vector = output.data[4];
	auto &physical_min_vector = output.data[5];
	auto &physical_max_vector = output.data[6];
	auto &digital_min_vector = output.data[7];
	auto &digital_max_vector = output.data[8];
	auto &prefiltering_vector = output.data[9];
	auto &samples_per_record_vector = output.data[10];
	auto &is_annotation_vector = output.data[11];

	auto filename_data = FlatVector::GetData<string_t>(filename_vector);
	auto label_data = FlatVector::GetData<string_t>(label_vector);
	auto transducer_type_data = FlatVector::GetData<string_t>(transducer_type_vector);
	auto physical_dim_data = FlatVector::GetData<string_t>(physical_dim_vector);
	auto prefiltering_data = FlatVector::GetData<string_t>(prefiltering_vector);

	auto &fs = FileSystem::GetFileSystem(context);

	idx_t actual_count = 0;
	idx_t num_signals_acc = 0;
	for (idx_t i = 0; i < count; i++) {
		idx_t file_index = start_idx + i;

		auto filename = bind_data.files[file_index].path;
		auto file_handle = fs.OpenFile(filename, FileOpenFlags::FILE_FLAGS_READ);
		auto signal_headers = BiosignalsEDFParser::ParseSignalHeaders(*file_handle);
		size_t num_signals = signal_headers.size();

		for (idx_t j = 0; j < num_signals; j++) {
			idx_t idx_eff = num_signals_acc + j;

			filename_data[idx_eff] = StringVector::AddString(filename_vector, filename);

			signal_id_vector.SetValue(idx_eff, Value::UINTEGER(j + 1));

			SetEmptyStringFieldToNull(signal_headers[j].label, label_vector, label_data, idx_eff);
			SetEmptyStringFieldToNull(signal_headers[j].transducer_type, transducer_type_vector, transducer_type_data,
			                          idx_eff);
			SetEmptyStringFieldToNull(signal_headers[j].physical_dim, physical_dim_vector, physical_dim_data, idx_eff);

			physical_min_vector.SetValue(idx_eff, Value::FLOAT(signal_headers[j].physical_min));
			physical_max_vector.SetValue(idx_eff, Value::FLOAT(signal_headers[j].physical_max));

			digital_min_vector.SetValue(idx_eff, Value::INTEGER(signal_headers[j].digital_min));
			digital_max_vector.SetValue(idx_eff, Value::INTEGER(signal_headers[j].digital_max));

			SetEmptyStringFieldToNull(signal_headers[j].prefiltering, prefiltering_vector, prefiltering_data, idx_eff);

			samples_per_record_vector.SetValue(idx_eff, Value::UINTEGER(signal_headers[j].samples_per_record));

			is_annotation_vector.SetValue(idx_eff, Value::BOOLEAN(signal_headers[j].is_annotation));

			actual_count += 1;
		}
		num_signals_acc += num_signals;
	}

	output.SetCardinality(actual_count);
}

unique_ptr<NodeStatistics> ReadEDFSignalHeadersCardinality(ClientContext &context, const FunctionData *bind_data_p) {
	return ReadEDFGlobalHeaderCardinality(context, bind_data_p);
}

double ReadEDFSignalHeadersProgress(ClientContext &context, const FunctionData *bind_data_p,
                                    const GlobalTableFunctionState *global_state_p) {
	return ReadEDFGlobalHeaderProgress(context, bind_data_p, global_state_p);
}

void RegisterReadEDFSignalHeaders(ExtensionLoader &loader) {
	TableFunction read_edf_signal_headers_func("read_edf_signal_headers", {LogicalType::VARCHAR},
	                                           ReadEDFSignalHeadersFunc, ReadEDFSignalHeadersFuncBind,
	                                           ReadEDFSignalHeadersGlobalInit, ReadEDFSignalHeadersLocalInit);

	CreateTableFunctionInfo read_edf_signal_headers_info(read_edf_signal_headers_func);
	FunctionDescription read_edf_signal_headers_desc;

	read_edf_signal_headers_desc.parameter_types = {LogicalType::VARCHAR};
	read_edf_signal_headers_desc.parameter_names = {"path"};
	read_edf_signal_headers_desc.description =
	    "Read signal information from the headers of EDF(+) / BDF(+) files into DuckDB.";
	read_edf_signal_headers_desc.examples = {"FROM read_edf_signal_headers('/path/to/recording.edf');",
	                                         "FROM read_edf_signal_headers('/path/to/all_recordings/*');"};
	read_edf_signal_headers_desc.categories = {"medical"};
	read_edf_signal_headers_info.descriptions.push_back(read_edf_signal_headers_desc);

	read_edf_signal_headers_func.cardinality = ReadEDFSignalHeadersCardinality;
	read_edf_signal_headers_func.table_scan_progress = ReadEDFSignalHeadersProgress;

	loader.RegisterFunction(read_edf_signal_headers_info);
}

} // namespace duckdb
