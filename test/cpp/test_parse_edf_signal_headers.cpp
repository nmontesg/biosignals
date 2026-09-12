#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	std::vector<duckdb::EDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseSignalHeaders(fh);

	std::vector<duckdb::EDFSignalHeader> expected_headers = {
	    duckdb::EDFSignalHeader("F4", "AgAgCl", "uV", -3200.0, 3200.0, -32768, 32767, "HP:0.015Hz", 200, "", false),
	    duckdb::EDFSignalHeader("F3", "AgAgCl", "uV", -3200.0, 3200.0, -32768, 32767, "HP:0.015Hz", 100, "", false),
	    duckdb::EDFSignalHeader("X10", "AgAgCl", "mV", -1.6, 1.6, -32768, 32767, "HP:0.015Hz", 200, "", false)};

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	return 0;
}
