#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	std::vector<duckdb::EDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseSignalHeaders(fh);

	std::vector<duckdb::EDFSignalHeader> expected_headers = {
	    duckdb::EDFSignalHeader("A1", "Active Electrode, pin type", "uV", -262144.0, 262144.0, -8388608, 8388607,
	                            "HP: DC; LP: 500 Hz", 2048, "Reserved", false),
	    duckdb::EDFSignalHeader("A2", "Active Electrode, pin type", "uV", -262144.0, 262144.0, -8388608, 8388607,
	                            "HP: DC; LP: 500 Hz", 2048, "Reserved", false),
	    duckdb::EDFSignalHeader("A3", "Active Electrode, pin type", "uV", -262144.0, 262144.0, -8388608, 8388607,
	                            "HP: DC; LP: 500 Hz", 2048, "Reserved", false)};

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	return 0;
}
