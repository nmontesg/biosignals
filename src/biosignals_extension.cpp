#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp" // IWYU pragma: keep
#include "biosignals_extension.hpp"
#include "read_edf_annotations.hpp"
#include "read_edf_global_header.hpp"
#include "read_edf_signal_headers.hpp"
#include "read_edf_signal.hpp"

namespace duckdb {

static void LoadInternal(ExtensionLoader &loader) {
	RegisterReadEDFAnnotations(loader);
	RegisterReadEDFGlobalHeader(loader);
	RegisterReadEDFSignalHeaders(loader);
	RegisterReadEDFSignal(loader);
}

void BiosignalsExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}
std::string BiosignalsExtension::Name() {
	return "biosignals";
}

std::string BiosignalsExtension::Version() const {
#ifdef EXT_VERSION_BIOSIGNALS
	return EXT_VERSION_BIOSIGNALS;
#else
	return "0.1.0";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(biosignals, loader) {
	duckdb::LoadInternal(loader);
}
}
