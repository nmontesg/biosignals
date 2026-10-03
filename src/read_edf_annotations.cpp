#include "biosignals_edf_parser.hpp"
#include "duckdb.hpp" // IWYU pragma: keep
#include "duckdb/common/types.hpp"
#include "duckdb/common/types/string_type.hpp"
#include "duckdb/common/vector/flat_vector.hpp"
#include "duckdb/common/vector/list_vector.hpp"
#include "duckdb/parser/parsed_data/create_function_info.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "read_edf_annotations.hpp"
#include "read_edf_signal.hpp"
#include <numeric>
#include <string>

namespace duckdb {
unique_ptr<FunctionData> ReadEDFAnnotationsFuncBind(ClientContext &context, TableFunctionBindInput &input,
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

	auto result = make_uniq<ReadEDFAnnotationsBindData>();
	result->files = std::move(file_list);

	names.push_back("filename");
	return_types.push_back(LogicalType::VARCHAR);

	names.push_back("record_id");
	return_types.push_back(LogicalType::UINTEGER);

	names.push_back("signal_id");
	return_types.push_back(LogicalType::UINTEGER);

	names.push_back("tal_id");
	return_types.push_back(LogicalType::UINTEGER);

	names.push_back("onset");
	return_types.push_back(LogicalType::DOUBLE);

	names.push_back("duration");
	return_types.push_back(LogicalType::DOUBLE);

	names.push_back("annotations");
	return_types.push_back(LogicalType::LIST(LogicalType::VARCHAR));

	return std::move(result);
}

unique_ptr<GlobalTableFunctionState> ReadEDFAnnotationsGlobalInit(ClientContext &context,
                                                                  TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<ReadEDFAnnotationsBindData>();
	return make_uniq<ReadEDFAnnotationsGlobalState>(context, bind_data.files);
}

unique_ptr<LocalTableFunctionState> ReadEDFAnnotationsLocalInit(ExecutionContext &context,
                                                                TableFunctionInitInput &input,
                                                                GlobalTableFunctionState *global_state_p) {
	return make_uniq<ReadEDFAnnotationsLocalState>();
}

void ReadEDFAnnotationsFunc(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &gstate = data.global_state->Cast<ReadEDFAnnotationsGlobalState>();
	auto &lstate = data.local_state->Cast<ReadEDFAnnotationsLocalState>();

	if (!lstate.has_work) {
		lstate.has_work = gstate.GetWorkItem(lstate.file_idx, lstate.start_record_idx, lstate.stop_record_idx);

		if (!lstate.has_work) {
			output.SetChildCardinality(0);
			return;
		}

		CopyRecordDataIntoLocalBuffer<true>(gstate, lstate);

		// the file_idx is guaranteed to have some annotation signal
		lstate.curr_record_idx = lstate.start_record_idx;
		lstate.curr_signal_idx = 0;

		FindFirstAnnotationSignal(gstate.signal_headers[lstate.file_idx], lstate.curr_signal_idx);
	}

	vector<TimestampedAnnotationList> tals = GetSignalAnnotations<true>(gstate, lstate, lstate.curr_signal_idx);
	size_t num_tals = tals.size();
	if (num_tals > STANDARD_VECTOR_SIZE) {
		throw IOException("Cannot parse EDF annotations with " + to_string(tals.size()) +
		                  " TALs in an annotation signal, " + gstate.files[lstate.file_idx].path);
	}

	// populate the columns
	string filename = gstate.files[lstate.file_idx].path;

	auto &filename_vector = output.data[0];
	auto &record_id_vector = output.data[1];
	auto &signal_id_vector = output.data[2];

	ConstantVector::Reference(filename_vector, Value(filename), count_t(num_tals));
	ConstantVector::Reference(record_id_vector, Value::UINTEGER(lstate.curr_record_idx + 1), count_t(num_tals));
	ConstantVector::Reference(signal_id_vector, Value::UINTEGER(lstate.curr_signal_idx + 1), count_t(num_tals));

	auto &tal_id_vector = output.data[3];
	auto &onset_vector = output.data[4];
	auto &duration_vector = output.data[5];

	auto tal_id_data = FlatVector::GetDataMutable<uint32_t>(tal_id_vector);
	auto onset_data = FlatVector::GetDataMutable<double>(onset_vector);
	auto duration_data = FlatVector::GetDataMutable<double>(duration_vector);

	// annotations column is a list: the parent vector contains (offset, len) for each element
	// the child vector contains the actual values for all the lists (all concatenated)
	auto &annotations_vector = output.data[6];
	auto annotations_data = FlatVector::GetDataMutable<list_entry_t>(annotations_vector);

	size_t total_annotations =
	    std::accumulate(tals.begin(), tals.end(), 0, [](int existing, TimestampedAnnotationList &_tal) {
		    return existing + _tal.annotations.size();
	    });
	ListVector::Reserve(annotations_vector, total_annotations);

	auto &annotations_child_vector = ListVector::GetChildMutable(annotations_vector);
	auto annotations_child_data = FlatVector::GetDataMutable<string_t>(annotations_child_vector);

	idx_t list_offset = 0;
	for (idx_t i = 0; i < num_tals; i++) {
		tal_id_data[i] = i + 1;
		onset_data[i] = tals[i].onset;
		if (tals[i].has_duration) {
			duration_data[i] = tals[i].duration;
		} else {
			FlatVector::SetNull(duration_vector, i, true);
		}

		// parse the list of annotations
		if (tals[i].annotations.empty()) {
			FlatVector::SetNull(annotations_vector, i, true);
		} else {
			annotations_data[i].offset = list_offset;
			annotations_data[i].length = tals[i].annotations.size();
			for (idx_t j = 0; j < tals[i].annotations.size(); j++) {
				annotations_child_data[list_offset + j] =
				    StringVector::AddString(annotations_child_vector, tals[i].annotations[j]);
			}
			list_offset += tals[i].annotations.size();
		}
	}

	output.SetChildCardinality(num_tals);

	// prepare for the next vector
	idx_t num_signals = gstate.global_headers[lstate.file_idx].num_signals;
	lstate.curr_signal_idx += 1;
	while (lstate.curr_signal_idx < num_signals) {
		bool is_next_signal_annotation = gstate.signal_headers[lstate.file_idx][lstate.curr_signal_idx].is_annotation;
		if (is_next_signal_annotation) {
			break;
		}
		lstate.curr_signal_idx += 1;
	}

	if (lstate.curr_signal_idx >= num_signals) {
		lstate.curr_record_idx += 1;
		lstate.curr_signal_idx = 0;
		FindFirstAnnotationSignal(gstate.signal_headers[lstate.file_idx], lstate.curr_signal_idx);
	}
	if (lstate.curr_record_idx >= lstate.stop_record_idx) {
		lstate.has_work = false;
	}
}

double ReadEDFAnnotationsProgress(ClientContext &context, const FunctionData *bind_data_p,
                                  const GlobalTableFunctionState *global_state_p) {
	return ReadEDFContentProgress<true>(context, bind_data_p, global_state_p);
}

void RegisterReadEDFAnnotations(ExtensionLoader &loader) {
	FunctionSignature read_edf_annotations_signature;
	read_edf_annotations_signature.AddArgs(Identifier("path"), LogicalType::VARCHAR);
	TableFunction read_edf_annotations_func(Identifier("read_edf_annotations"), read_edf_annotations_signature,
	                                        ReadEDFAnnotationsFunc, ReadEDFAnnotationsFuncBind,
	                                        ReadEDFAnnotationsGlobalInit, ReadEDFAnnotationsLocalInit);
	read_edf_annotations_func.order_preservation_type = OrderPreservationType::NO_ORDER;

	CreateTableFunctionInfo read_edf_annotations_info(read_edf_annotations_func);
	FunctionDescription read_edf_annotations_desc;

	read_edf_annotations_desc.description = "Read annotation signals from EDF(+) / BDF(+) files into DuckDB.";
	read_edf_annotations_desc.examples = {"FROM read_edf_annotations('/path/to/recording.edf');",
	                                      "FROM read_edf_annotations('/path/to/all_recordings/*');"};
	read_edf_annotations_desc.categories = {"medical"};
	read_edf_annotations_info.descriptions.push_back(read_edf_annotations_desc);

	read_edf_annotations_func.table_scan_progress = ReadEDFAnnotationsProgress;

	loader.RegisterFunction(read_edf_annotations_info);
}

} // namespace duckdb
