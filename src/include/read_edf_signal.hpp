#pragma once

#include "duckdb.hpp" // IWYU pragma: keep
#include "read_edf_content.hpp"

namespace duckdb {

struct ReadEDFSignalBindData : public ReadEDFContentBindData {};

unique_ptr<FunctionData> ReadEDFSignalFuncBind(ClientContext &, TableFunctionBindInput &, vector<LogicalType> &,
                                               vector<Identifier> &);

struct ReadEDFSignalGlobalState : public ReadEDFContentGlobalState<false> {
	explicit ReadEDFSignalGlobalState(ClientContext &_context, const vector<OpenFileInfo> &_files)
	    : duckdb::ReadEDFContentGlobalState<false>(_context, _files) {
	}
};

struct ReadEDFSignalLocalState : public ReadEDFContentLocalState {
	explicit ReadEDFSignalLocalState() : ReadEDFContentLocalState() {
	}
};

unique_ptr<GlobalTableFunctionState> ReadEDFSignalGlobalInit(ClientContext &, TableFunctionInitInput &);
unique_ptr<LocalTableFunctionState> ReadEDFSignalLocalInit(ExecutionContext &, TableFunctionInitInput &,
                                                           GlobalTableFunctionState *);

void FindFirstNonAnnotationSignal(const vector<EDFSignalHeader> &, idx_t &);
void FindFirstAnnotationSignal(const vector<EDFSignalHeader> &, idx_t &);
void GetCurrentRecordOffset(const ReadEDFSignalGlobalState &, ReadEDFSignalLocalState &);

void ReadEDFSignalFunc(ClientContext &, TableFunctionInput &, DataChunk &);
double ReadEDFSignalProgress(ClientContext &, const FunctionData *, const GlobalTableFunctionState *);
void RegisterReadEDFSignal(ExtensionLoader &);

} // namespace duckdb
