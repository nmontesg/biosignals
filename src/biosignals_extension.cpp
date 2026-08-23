#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp" // IWYU pragma: keep
#include "biosignals_extension.hpp"

namespace duckdb {

inline void BiosignalsScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &name_vector = args.data[0];
	UnaryExecutor::Execute<string_t, string_t>(name_vector, result, args.size(), [&](string_t name) {
		return StringVector::AddString(result, "...........🦆 " + name.GetString());
	});
}

static void LoadInternal(ExtensionLoader &loader) {
	// TODO remove
	// Register a scalar function
	auto biosignals_scalar_function =
	    ScalarFunction("biosignals", {LogicalType::VARCHAR}, LogicalType::VARCHAR, BiosignalsScalarFun);

	loader.RegisterFunction(biosignals_scalar_function);
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
