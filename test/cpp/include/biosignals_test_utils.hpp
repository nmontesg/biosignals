#pragma once

#include "biosignals_win32.hpp""

#include "biosignals_edf_parser.hpp"
#include "duckdb/main/database.hpp"
#include <sstream>
#include <string>

class BIOSIGNALS_EXPORT BiosignalsTestUtils {
public:
	static duckdb::unique_ptr<duckdb::CachingFileHandle> GetCachingFileHandle(duckdb::DuckDB &, const char *);

	static void AssertStringEquals(const char *, const char *);
	static void AssertStringEquals(const std::string &, const std::string &);

	template <typename T>
	static void AssertEquals(const T actual, const T expected) {
		if (actual != expected) {
			std::ostringstream oss;
			oss << "Actual value (" << actual << ") does not match expected value (" << expected << ")";
			throw std::runtime_error(oss.str());
		}
	}

	static void CompareRawEDFGlobalHeader(const duckdb::RawEDFGlobalHeader *, const duckdb::RawEDFGlobalHeader *,
	                                      const bool = false);
	static void CompareEDFGlobalHeader(const duckdb::EDFGlobalHeader *, const duckdb::EDFGlobalHeader *);

	static void CompareRawEDFSignalHeader(const duckdb::RawEDFSignalHeader *, const duckdb::RawEDFSignalHeader *);
	static void CompareEDFSignalHeader(const duckdb::EDFSignalHeader *, const duckdb::EDFSignalHeader *);
};
