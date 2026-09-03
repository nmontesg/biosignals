#pragma once

#include "duckdb.hpp" // IWYU pragma: keep
#include "read_edf_global_header.hpp"

namespace duckdb {

struct ReadEDFSignalHeadersBindData : public ReadEDFGlobalHeaderBindData {};

unique_ptr<FunctionData> ReadEDFSignalHeadersFuncBind(ClientContext &, TableFunctionBindInput &, vector<LogicalType> &,
                                                      vector<string> &);

struct ReadEDFSignalHeadersGlobalState : public ReadEDFGlobalHeaderGlobalState {
	explicit ReadEDFSignalHeadersGlobalState(int64_t total_files) : ReadEDFGlobalHeaderGlobalState(total_files) {};
};

struct ReadEDFSignalHeadersLocalState : public ReadEDFGlobalHeaderLocalState {};

unique_ptr<GlobalTableFunctionState> ReadEDFSignalHeadersGlobalInit(ClientContext &, TableFunctionInitInput &);
unique_ptr<LocalTableFunctionState> ReadEDFSignalHeadersLocalInit(ExecutionContext &, TableFunctionInitInput &,
                                                                  GlobalTableFunctionState *);
void ReadEDFSignalHeadersFunc(ClientContext &, TableFunctionInput &, DataChunk &);
unique_ptr<NodeStatistics> ReadEDFSignalHeadersCardinality(ClientContext &, const FunctionData *);
double ReadEDFSignalHeadersProgress(ClientContext &, const FunctionData *, const GlobalTableFunctionState *);
void RegisterReadEDFSignalHeaders(ExtensionLoader &);

} // namespace duckdb
