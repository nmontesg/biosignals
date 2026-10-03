#pragma once

#include "duckdb.hpp" // IWYU pragma: keep
#include "read_edf_content.hpp"

namespace duckdb {

struct ReadEDFAnnotationsBindData : public ReadEDFContentBindData {};

unique_ptr<FunctionData> ReadEDFAnnotationsFuncBind(ClientContext &, TableFunctionBindInput &, vector<LogicalType> &,
                                                    vector<Identifier> &);

struct ReadEDFAnnotationsGlobalState : public ReadEDFContentGlobalState<true> {
	explicit ReadEDFAnnotationsGlobalState(ClientContext &_context, const vector<OpenFileInfo> &_files)
	    : duckdb::ReadEDFContentGlobalState<true>(_context, _files) {
	}
};

struct ReadEDFAnnotationsLocalState : public ReadEDFContentLocalState {
	explicit ReadEDFAnnotationsLocalState() : ReadEDFContentLocalState() {
	}
};

unique_ptr<GlobalTableFunctionState> ReadEDFAnnotationsGlobalInit(ClientContext &, TableFunctionInitInput &);
unique_ptr<LocalTableFunctionState> ReadEDFAnnotationsLocalInit(ExecutionContext &, TableFunctionInitInput &,
                                                                GlobalTableFunctionState *);
void ReadEDFAnnotationsFunc(ClientContext &, TableFunctionInput &, DataChunk &);
double ReadEDFAnnotationsProgress(ClientContext &, const FunctionData *, const GlobalTableFunctionState *);
void RegisterReadEDFAnnotations(ExtensionLoader &);

} // namespace duckdb
