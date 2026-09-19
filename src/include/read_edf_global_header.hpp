#pragma once

#include "duckdb.hpp" // IWYU pragma: keep
#include "duckdb/storage/external_file_cache/caching_file_system.hpp"

namespace duckdb {

struct ReadEDFGlobalHeaderBindData : public TableFunctionData {
	vector<OpenFileInfo> files;
};

unique_ptr<FunctionData> ReadEDFGlobalHeaderFuncBind(ClientContext &, TableFunctionBindInput &, vector<LogicalType> &,
                                                     vector<Identifier> &);

struct ReadEDFGlobalHeaderGlobalState : public GlobalTableFunctionState {
	std::mutex mutex;
	CachingFileSystem filesystem;
	idx_t num_files_left_to_read;
	const idx_t total_files;

	explicit ReadEDFGlobalHeaderGlobalState(ClientContext &contx, int64_t total_files)
	    : GlobalTableFunctionState(), filesystem(CachingFileSystem::Get(contx)), num_files_left_to_read(total_files),
	      total_files(total_files) {
	}

	bool GetWorkItem(idx_t &start_idx, idx_t &end_idx) {
		std::lock_guard<std::mutex> lock(mutex);
		if (num_files_left_to_read == 0) {
			return false;
		}
		start_idx = total_files - num_files_left_to_read;
		idx_t work_size = MinValue<idx_t>(num_files_left_to_read, STANDARD_VECTOR_SIZE);
		end_idx = start_idx + work_size;
		num_files_left_to_read -= work_size;
		return true;
	}

	idx_t MaxThreads() const override {
		return GlobalTableFunctionState::MAX_THREADS;
	}
};

struct ReadEDFGlobalHeaderLocalState : public LocalTableFunctionState {
	explicit ReadEDFGlobalHeaderLocalState() : LocalTableFunctionState() {
	}
};

unique_ptr<GlobalTableFunctionState> ReadEDFGlobalHeaderGlobalInit(ClientContext &, TableFunctionInitInput &);
unique_ptr<LocalTableFunctionState> ReadEDFGlobalHeaderLocalInit(ExecutionContext &, TableFunctionInitInput &,
                                                                 GlobalTableFunctionState *);
void SetEmptyStringFieldToNull(const string &, Vector &, string_t *, const idx_t);
void ReadEDFGlobalHeaderFunc(ClientContext &, TableFunctionInput &, DataChunk &);
unique_ptr<NodeStatistics> ReadEDFGlobalHeaderCardinality(ClientContext &, const FunctionData *);
double ReadEDFGlobalHeaderProgress(ClientContext &, const FunctionData *, const GlobalTableFunctionState *);
void RegisterReadEDFGlobalHeader(ExtensionLoader &);

} // namespace duckdb
