#pragma once

#include <thread>

#include "duckdb.hpp" // IWYU pragma: keep
#include "biosignals_edf_parser.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {

struct ReadEDFContentBindData : public TableFunctionData {
	vector<OpenFileInfo> files;
};

template <bool ReadAnnotations>
struct ReadEDFContentGlobalState : public GlobalTableFunctionState {
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

	explicit ReadEDFContentGlobalState(ClientContext &_context, const vector<OpenFileInfo> &_files)
	    : GlobalTableFunctionState(), filesystem(CachingFileSystem::Get(_context)), files(_files),
	      num_files(_files.size()) {
		global_headers.resize(num_files);
		signal_headers.resize(num_files);

		// populate info to start tracking the first file
		next_file_idx = 0;
		next_rec_start_idx = 0;
		PrepareDataForFile(0);

		// if there are no relevant signals to read, move on to the next file
		idx_t num_relevant_signals;
		if (ReadAnnotations) {
			num_relevant_signals = BiosignalsEDFParser::GetNumberOfAnnotationSignals(signal_headers[next_file_idx]);
		} else {
			num_relevant_signals = BiosignalsEDFParser::GetNumberOfNonAnnotationSignals(signal_headers[next_file_idx]);
		}
		if (num_relevant_signals == 0) {
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

		// handle files where all the signals are irrelevant
		idx_t num_relevant_signals;
		if (ReadAnnotations) {
			num_relevant_signals = BiosignalsEDFParser::GetNumberOfAnnotationSignals(signal_headers[next_file_idx]);
		} else {
			num_relevant_signals = BiosignalsEDFParser::GetNumberOfNonAnnotationSignals(signal_headers[next_file_idx]);
		}
		if (num_relevant_signals > 0) {
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
	}
};

struct ReadEDFContentLocalState : public LocalTableFunctionState {
	bool has_work;
	vector<unsigned char> buffer;

	// unit of work
	idx_t file_idx;
	idx_t start_record_idx;
	idx_t stop_record_idx;

	// track the current work being processed
	idx_t curr_record_idx;
	idx_t curr_signal_idx;
	idx_t curr_start_timestep;
	float curr_record_onset;

	explicit ReadEDFContentLocalState() : LocalTableFunctionState(), has_work(false) {
	}
};

template <bool ReadAnnotations>
void CopyRecordDataIntoLocalBuffer(ReadEDFContentGlobalState<ReadAnnotations> &gstate,
                                   ReadEDFContentLocalState &lstate) {
	EDFGlobalHeader gheader = gstate.global_headers[lstate.file_idx];
	vector<EDFSignalHeader> sigheaders = gstate.signal_headers[lstate.file_idx];

	// read the data in bulk here
	bool is_bdf = gheader.is_bdf;
	size_t bytes_per_record;
	if (is_bdf) {
		bytes_per_record = BiosignalsEDFParser::GetRecordSize<3>(sigheaders);
	} else {
		bytes_per_record = BiosignalsEDFParser::GetRecordSize<2>(sigheaders);
	}

	size_t num_records = lstate.stop_record_idx - lstate.start_record_idx;
	size_t num_bytes_to_copy = num_records * bytes_per_record;
	lstate.buffer.resize(num_bytes_to_copy);

	idx_t offset =
	    BiosignalsEDFParser::GetTotalHeaderSize(gheader.num_signals) + bytes_per_record * lstate.start_record_idx;

	auto filehandle = gstate.filesystem.OpenFile(gstate.files[lstate.file_idx], FileOpenFlags::FILE_FLAGS_READ);
	auto shared_filehandle = filehandle->GetFileHandle();
	shared_filehandle->Read(lstate.buffer.data(), num_bytes_to_copy, offset);
}

template <bool ReadAnnotations>
idx_t GetSignalOffsetInRecord(const ReadEDFContentGlobalState<ReadAnnotations> &gstate,
                              const ReadEDFContentLocalState &lstate, const idx_t sig_idx) {
	bool is_bdf = gstate.global_headers[lstate.file_idx].is_bdf;
	size_t signal_offset_in_record;
	if (is_bdf) {
		signal_offset_in_record =
		    BiosignalsEDFParser::GetSignalOffsetWithinRecord<3>(sig_idx, gstate.signal_headers[lstate.file_idx]);
	} else {
		signal_offset_in_record =
		    BiosignalsEDFParser::GetSignalOffsetWithinRecord<2>(sig_idx, gstate.signal_headers[lstate.file_idx]);
	}
	return signal_offset_in_record;
}

template <bool ReadAnnotations>
idx_t GetBytesPerRecord(const ReadEDFContentGlobalState<ReadAnnotations> &gstate,
                        const ReadEDFContentLocalState &lstate) {
	size_t bytes_per_record;
	bool is_bdf = gstate.global_headers[lstate.file_idx].is_bdf;
	if (is_bdf) {
		bytes_per_record = BiosignalsEDFParser::GetRecordSize<3>(gstate.signal_headers[lstate.file_idx]);
	} else {
		bytes_per_record = BiosignalsEDFParser::GetRecordSize<2>(gstate.signal_headers[lstate.file_idx]);
	}
	return bytes_per_record;
}

template <bool ReadAnnotations>
vector<TimestampedAnnotationList> GetSignalAnnotations(const ReadEDFContentGlobalState<ReadAnnotations> &gstate,
                                                       const ReadEDFContentLocalState &lstate, idx_t sig_idx) {
	size_t bytes_per_record = GetBytesPerRecord<ReadAnnotations>(gstate, lstate);
	size_t signal_offset_in_record = GetSignalOffsetInRecord<ReadAnnotations>(gstate, lstate, sig_idx);

	bool is_bdf = gstate.global_headers[lstate.file_idx].is_bdf;
	size_t bytes_per_sample = (is_bdf ? 3 : 2);
	int64_t buffer_offset =
	    (int64_t)(bytes_per_record * (lstate.curr_record_idx - lstate.start_record_idx) + signal_offset_in_record);

	// parse the TALs
	int64_t annotations_size =
	    (int64_t)(gstate.signal_headers[lstate.file_idx][sig_idx].samples_per_record * bytes_per_sample);
	std::string annotation_data(lstate.buffer.begin() + buffer_offset,
	                            lstate.buffer.begin() + buffer_offset + annotations_size);
	vector<TimestampedAnnotationList> tals = BiosignalsEDFParser::ParseRecordTALs(annotation_data);

	return tals;
}

template <bool ReadAnnotations>
double ReadEDFContentProgress(ClientContext &context, const FunctionData *bind_data_p,
                              const GlobalTableFunctionState *global_state_p) {
	auto &bind_data = bind_data_p->Cast<ReadEDFContentBindData>();
	auto &gstate = global_state_p->Cast<ReadEDFContentGlobalState<ReadAnnotations>>();

	if (bind_data.files.empty()) {
		return 100.0;
	}

	float contribution_per_file = 1 / static_cast<float>(gstate.num_files);
	size_t curr_num_records = gstate.global_headers[gstate.next_file_idx].num_records;
	float curr_file_progress = static_cast<float>(gstate.next_rec_start_idx) / static_cast<float>(curr_num_records);

	float progress = contribution_per_file * (static_cast<float>(gstate.next_file_idx) + curr_file_progress);
	return progress * 100.0;
}

} // namespace duckdb
