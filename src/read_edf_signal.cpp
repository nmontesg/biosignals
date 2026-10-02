#include "biosignals_edf_parser.hpp"
#include "boost/endian/conversion.hpp"
#include "duckdb.hpp" // IWYU pragma: keep
#include "duckdb/common/enums/order_preservation_type.hpp"
#include "duckdb/common/helper.hpp"
#include "duckdb/common/typedefs.hpp"
#include "duckdb/common/vector_size.hpp"
#include "duckdb/parser/parsed_data/create_function_info.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "read_edf_signal.hpp"

namespace duckdb {

unique_ptr<FunctionData> ReadEDFSignalFuncBind(ClientContext &context, TableFunctionBindInput &input,
                                               vector<LogicalType> &return_types, vector<Identifier> &names) {
	if (input.inputs.empty()) {
		throw InvalidInputException("read_edf_signal requires at least one argument.");
	}

	auto &path_param = input.inputs[0];
	string glob_pattern = StringValue::Get(path_param);
	auto &fs = FileSystem::GetFileSystem(context);
	auto file_list = fs.Glob(glob_pattern);
	if (file_list.empty()) {
		throw IOException("No files found matching the provided pattern.");
	}

	auto result = make_uniq<ReadEDFSignalBindData>();
	result->files = std::move(file_list);

	names.push_back("filename");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("record_id");
	return_types.push_back(LogicalType::UINTEGER);

	names.push_back("signal_id");
	return_types.push_back(LogicalType::UINTEGER);

	names.push_back("timestep");
	return_types.push_back(LogicalType::UINTEGER);

	names.push_back("time");
	return_types.push_back(LogicalType::DOUBLE);

	names.push_back("digital_signal");
	return_types.push_back(LogicalType::INTEGER);

	names.push_back("physical_signal");
	return_types.push_back(LogicalType::FLOAT);

	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> ReadEDFSignalGlobalInit(ClientContext &context, TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<ReadEDFSignalBindData>();
	return make_uniq<ReadEDFSignalGlobalState>(context, bind_data.files);
}

unique_ptr<LocalTableFunctionState> ReadEDFSignalLocalInit(ExecutionContext &context, TableFunctionInitInput &input,
                                                           GlobalTableFunctionState *global_state_p) {
	return make_uniq<ReadEDFSignalLocalState>();
}

// void CopyRecordDataIntoLocalBuffer(ReadEDFSignalGlobalState &gstate, ReadEDFSignalLocalState &lstate) {
void CopyRecordDataIntoLocalBuffer(EDFGlobalHeader &gheader, vector<EDFSignalHeader> &sigheaders, const idx_t from_rec,
                                   const idx_t to_rec, vector<unsigned char> &buff, CachingFileSystem &fs,
                                   const OpenFileInfo &fi) {
	// read the data in bulk here
	bool is_bdf = gheader.is_bdf;
	size_t bytes_per_record;
	if (is_bdf) {
		bytes_per_record = BiosignalsEDFParser::GetRecordSize<3>(sigheaders);
	} else {
		bytes_per_record = BiosignalsEDFParser::GetRecordSize<2>(sigheaders);
	}

	size_t num_records = to_rec - from_rec; // lstate.stop_record_idx - lstate.start_record_idx
	size_t num_bytes_to_copy = num_records * bytes_per_record;
	buff.resize(num_bytes_to_copy);

	idx_t offset = BiosignalsEDFParser::GetTotalHeaderSize(gheader.num_signals) + bytes_per_record * from_rec;

	auto filehandle = fs.OpenFile(fi, FileOpenFlags::FILE_FLAGS_READ);
	auto filehandle_group = filehandle->Read(num_bytes_to_copy, offset);
	filehandle_group.CopyTo(buff.data(), num_bytes_to_copy);
}

void FindFirstNonAnnotationSignal(vector<EDFSignalHeader> &sigheaders, idx_t &signal_idx) {
	bool is_annotation_signal = sigheaders[signal_idx].is_annotation;
	while (is_annotation_signal) {
		signal_idx += 1;
		is_annotation_signal = sigheaders[signal_idx].is_annotation;
	}
}

void ReadEDFSignalFunc(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &gstate = data.global_state->Cast<ReadEDFSignalGlobalState>();
	auto &lstate = data.local_state->Cast<ReadEDFSignalLocalState>();

	if (!lstate.has_work) {
		lstate.has_work = gstate.GetWorkItem(lstate.file_idx, lstate.start_record_idx, lstate.stop_record_idx);

		if (!lstate.has_work) {
			output.SetChildCardinality(0);
			return;
		}

		CopyRecordDataIntoLocalBuffer(gstate.global_headers[lstate.file_idx], gstate.signal_headers[lstate.file_idx],
		                              lstate.start_record_idx, lstate.stop_record_idx, lstate.buffer, gstate.filesystem,
		                              gstate.files[lstate.file_idx]);

		// the file_idx is guaranteed to have some non-annotation signal
		lstate.curr_record_idx = lstate.start_record_idx;
		lstate.curr_signal_idx = 0;
		lstate.curr_start_timestep = 0;

		FindFirstNonAnnotationSignal(gstate.signal_headers[lstate.file_idx], lstate.curr_signal_idx);
	}

	// the offset in the local buffer from which we need to start parsing samples
	bool is_bdf = gstate.global_headers[lstate.file_idx].is_bdf;
	size_t bytes_per_record;
	size_t signal_offset_in_record;
	if (is_bdf) {
		bytes_per_record = BiosignalsEDFParser::GetRecordSize<3>(gstate.signal_headers[lstate.file_idx]);
		signal_offset_in_record = BiosignalsEDFParser::GetSignalOffsetWithinRecord<3>(
		    lstate.curr_signal_idx, gstate.signal_headers[lstate.file_idx]);
	} else {
		bytes_per_record = BiosignalsEDFParser::GetRecordSize<2>(gstate.signal_headers[lstate.file_idx]);
		signal_offset_in_record = BiosignalsEDFParser::GetSignalOffsetWithinRecord<2>(
		    lstate.curr_signal_idx, gstate.signal_headers[lstate.file_idx]);
	}
	size_t bytes_per_sample = (is_bdf ? 3 : 2);
	idx_t buffer_offset = bytes_per_record * (lstate.curr_record_idx - lstate.start_record_idx) +
	                      signal_offset_in_record + lstate.curr_start_timestep * bytes_per_sample;

	// until which sample do we need to parse
	size_t signal_samples_per_record =
	    gstate.signal_headers[lstate.file_idx][lstate.curr_signal_idx].samples_per_record;
	idx_t curr_stop_timestep =
	    MinValue<idx_t>(lstate.curr_start_timestep + STANDARD_VECTOR_SIZE, signal_samples_per_record);
	count_t num_timesteps(curr_stop_timestep - lstate.curr_start_timestep);

	// populate the columns
	string filename = gstate.files[lstate.file_idx].path;

	auto &filename_vector = output.data[0];
	auto &record_id_vector = output.data[1];
	auto &signal_id_vector = output.data[2];

	ConstantVector::Reference(filename_vector, Value(filename), num_timesteps);
	ConstantVector::Reference(record_id_vector, Value::UINTEGER(lstate.curr_record_idx + 1), num_timesteps);
	ConstantVector::Reference(signal_id_vector, Value::UINTEGER(lstate.curr_signal_idx + 1), num_timesteps);

	auto &timestep_vector = output.data[3];
	auto &time_vector = output.data[4];
	auto &digital_signal_vector = output.data[5];
	auto &physical_signal_vector = output.data[6];

	auto timestep_data = FlatVector::GetDataMutable<uint32_t>(timestep_vector);
	auto time_data = FlatVector::GetDataMutable<double>(time_vector);
	auto digital_signal_data = FlatVector::GetDataMutable<int32_t>(digital_signal_vector);
	auto physical_signal_data = FlatVector::GetDataMutable<float>(physical_signal_vector);

	double period = gstate.global_headers[lstate.file_idx].duration_record / (double)signal_samples_per_record;
	idx_t timestep_start =
	    lstate.curr_record_idx * gstate.signal_headers[lstate.file_idx][lstate.curr_signal_idx].samples_per_record +
	    lstate.curr_start_timestep;
	// TODO this needs to adapt to handle discontinuous filed (EDF+D, BDF+D)
	double time_start = (double)lstate.curr_record_idx * gstate.global_headers[lstate.file_idx].duration_record +
	                    period * (double)lstate.curr_start_timestep;
	if (gstate.global_headers[lstate.file_idx].is_discontinuous) {
		// time_start = ...;
	}

	float physical_offset = gstate.signal_headers[lstate.file_idx][lstate.curr_signal_idx].offset;
	float physical_slope = gstate.signal_headers[lstate.file_idx][lstate.curr_signal_idx].slope;

	idx_t _buff_off = buffer_offset;
	for (idx_t i = 0; i < num_timesteps; i++) {
		timestep_data[i] = i + timestep_start;
		time_data[i] = time_start + (float)i * period;
		if (is_bdf) {
			digital_signal_data[i] = boost::endian::load_little_s24(lstate.buffer.data() + _buff_off);
		} else {
			digital_signal_data[i] = boost::endian::load_little_s16(lstate.buffer.data() + _buff_off);
		}
		physical_signal_data[i] = (float)digital_signal_data[i] * physical_slope + physical_offset;
		_buff_off += bytes_per_sample;
	}

	output.SetChildCardinality(num_timesteps);

	// prepare for the next vector
	lstate.curr_start_timestep = curr_stop_timestep;
	idx_t num_signals = gstate.global_headers[lstate.file_idx].num_signals;
	if (lstate.curr_start_timestep >= signal_samples_per_record) {
		lstate.curr_signal_idx += 1;
		lstate.curr_start_timestep = 0;

		while (lstate.curr_signal_idx < num_signals) {
			bool is_next_signal_annotation =
			    gstate.signal_headers[lstate.file_idx][lstate.curr_signal_idx].is_annotation;
			if (!is_next_signal_annotation) {
				break;
			}
			lstate.curr_signal_idx += 1;
		}
	}

	if (lstate.curr_signal_idx >= num_signals) {
		lstate.curr_record_idx += 1;
		lstate.curr_signal_idx = 0;
		lstate.curr_start_timestep = 0;

		FindFirstNonAnnotationSignal(gstate.signal_headers[lstate.file_idx], lstate.curr_signal_idx);
	}
	if (lstate.curr_record_idx >= lstate.stop_record_idx) {
		lstate.has_work = false;
	}
}

double ReadEDFSignalProgress(ClientContext &context, const FunctionData *bind_data_p,
                             const GlobalTableFunctionState *global_state_p) {
	auto &bind_data = bind_data_p->Cast<ReadEDFSignalBindData>();
	auto &gstate = global_state_p->Cast<ReadEDFSignalGlobalState>();

	if (bind_data.files.empty()) {
		return 100.0;
	}

	float contribution_per_file = 1 / static_cast<float>(gstate.num_files);
	size_t curr_num_records = gstate.global_headers[gstate.next_file_idx].num_records;
	float curr_file_progress = static_cast<float>(gstate.next_rec_start_idx) / static_cast<float>(curr_num_records);

	float progress = contribution_per_file * (static_cast<float>(gstate.next_file_idx) + curr_file_progress);
	return progress * 100.0;
}

void RegisterReadEDFSignal(ExtensionLoader &loader) {
	FunctionSignature read_edf_signal_signature;
	read_edf_signal_signature.AddArgs(Identifier("path"), LogicalType::VARCHAR);
	TableFunction read_edf_signal_func(Identifier("read_edf_signal"), read_edf_signal_signature, ReadEDFSignalFunc,
	                                   ReadEDFSignalFuncBind, ReadEDFSignalGlobalInit, ReadEDFSignalLocalInit);
	read_edf_signal_func.order_preservation_type = OrderPreservationType::NO_ORDER;

	CreateTableFunctionInfo read_edf_signal_info(read_edf_signal_func);
	FunctionDescription read_edf_signal_desc;

	read_edf_signal_desc.description = "Read signal records from EDF(+) / BDF(+) files into DuckDB.";
	read_edf_signal_desc.examples = {"FROM read_edf_signal('/path/to/recording.edf');",
	                                 "FROM read_edf_signal('/path/to/all_recordings/*');"};
	read_edf_signal_desc.categories = {"medical"};
	read_edf_signal_info.descriptions.push_back(read_edf_signal_desc);

	read_edf_signal_func.table_scan_progress = ReadEDFSignalProgress;

	loader.RegisterFunction(read_edf_signal_info);
}

} // namespace duckdb
