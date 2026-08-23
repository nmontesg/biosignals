#pragma once

#include "duckdb.hpp" // IWYU pragma: keep

namespace duckdb {

class BiosignalsExtension : public Extension {
public:
	void Load(ExtensionLoader &db) override;
	std::string Name() override;
	std::string Version() const override;
};

} // namespace duckdb
