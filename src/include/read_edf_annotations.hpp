#pragma once

#include <thread>

#include "duckdb.hpp" // IWYU pragma: keep
#include "biosignals_edf_parser.hpp"
#include "read_edf_global_header.hpp"

namespace duckdb {

struct ReadEDFAnnotationsBindData : public ReadEDFGlobalHeaderBindData {};

unique_ptr<FunctionData> ReadEDFAnnotationsFuncBind(ClientContext &, TableFunctionBindInput &, vector<LogicalType> &,
                                                    vector<Identifier> &);

struct ReadEDFAnnotationsGlobalState : public GlobalTableFunctionState {
	std::mutex mutex;
	const unsigned int num_threads = std::thread::hardware_concurrency();

	CachingFileSystem filesystem;
	const vector<OpenFileInfo> &files;
	const size_t num_files;

	std::vector<EDFGlobalHeader> global_headers;
	std::vector<vector<EDFSignalHeader>> signal_headers;

	// for tracking the next batch of work to hand out
	idx_t next_file_idx;
	idx_t next_rec_start_idx;

	explicit ReadEDFAnnotationsGlobalState(ClientContext &_context, const vector<OpenFileInfo> &_files)
	    : GlobalTableFunctionState(), filesystem(CachingFileSystem::Get(_context)), files(_files),
	      num_files(_files.size()) {
		global_headers.resize(num_files);
		signal_headers.resize(num_files);

		// populate info to start tracking the first file
		next_file_idx = 0;
		next_rec_start_idx = 0;
		PrepareDataForFile(0);

		idx_t num_annotation_signals = BiosignalsEDFParser::GetNumberOfAnnotationSignals(signal_headers[next_file_idx]);
		if (num_annotation_signals == 0) {
			MoveToNextFile();
		}
	}

	void PrepareDataForFile(const idx_t file_idx) {
		auto fp = files[file_idx].path;
		auto file_handle = filesystem.OpenFile(fp, FileOpenFlags::FILE_FLAGS_READ);
		global_headers[file_idx] = BiosignalsEDFParser::ParseGlobalHeader(file_handle);
		signal_headers[file_idx] = BiosignalsEDFParser::ParseSignalHeaders(file_handle);
	}

	void MoveToNextFile() {
		next_file_idx += 1;
		next_rec_start_idx = 0;
		if (next_file_idx >= num_files) {
			return;
		}

		PrepareDataForFile(next_file_idx);

		// handle files where all the signals are annotations
		idx_t num_annotation_signals = BiosignalsEDFParser::GetNumberOfAnnotationSignals(signal_headers[next_file_idx]);
		if (num_annotation_signals > 0) {
			return;
		}

		MoveToNextFile();
	}

	bool GetWorkItem(idx_t &file_idx, idx_t &start_record_idx, idx_t &stop_record_idx) {
		std::lock_guard<std::mutex> lock(mutex);
		if (next_file_idx >= num_files) {
			return false;
		}
		file_idx = next_file_idx;
		start_record_idx = next_rec_start_idx;
		unsigned int num_records = global_headers[file_idx].num_records;
		unsigned int num_records_per_batch = MaxValue<unsigned int>(num_records / num_threads, 1);
		stop_record_idx = MinValue<idx_t>(start_record_idx + num_records_per_batch, num_records);

		// prepare for the next request for work
		if (stop_record_idx >= num_records) {
			MoveToNextFile();
		} else {
			next_rec_start_idx = stop_record_idx;
		}

		return true;
	}

	idx_t MaxThreads() const override {
		return GlobalTableFunctionState::MAX_THREADS;
		// TODO revert
		// return 1;
	}
};

struct ReadEDFAnnotationsLocalState : public LocalTableFunctionState {
	bool has_work;
	vector<unsigned char> buffer;

	// unit of work
	idx_t file_idx;
	idx_t start_record_idx;
	idx_t stop_record_idx;

	// track the current work being processed
	idx_t curr_record_idx;
	idx_t curr_signal_idx;

	explicit ReadEDFAnnotationsLocalState() : LocalTableFunctionState(), has_work(false) {
	}
};

unique_ptr<GlobalTableFunctionState> ReadEDFAnnotationsGlobalInit(ClientContext &, TableFunctionInitInput &);
unique_ptr<LocalTableFunctionState> ReadEDFAnnotationsLocalInit(ExecutionContext &, TableFunctionInitInput &,
                                                                GlobalTableFunctionState *);
void ReadEDFAnnotationsFunc(ClientContext &, TableFunctionInput &, DataChunk &);
void FindFirstAnnotationSignal(vector<EDFSignalHeader> &, idx_t &);
double ReadEDFAnnotationsProgress(ClientContext &, const FunctionData *, const GlobalTableFunctionState *);
void RegisterReadEDFAnnotations(ExtensionLoader &);

} // namespace duckdb
