#pragma once

#include "duckdb.hpp" // IWYU pragma: keep

#define EXTENSION_NAME "biosignals"

namespace duckdb {

class BiosignalsExtension : public Extension {
public:
	void Load(ExtensionLoader &db) override;
	std::string Name() override;
	std::string Version() const override;
};

} // namespace duckdb
