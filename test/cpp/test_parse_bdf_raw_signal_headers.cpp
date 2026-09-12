#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	std::vector<duckdb::RawEDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseRawSignalHeaders(fh);

	std::vector<duckdb::RawEDFSignalHeader> expected_headers;
	expected_headers.reserve(3);
	expected_headers.push_back(duckdb::RawEDFSignalHeader {
	    "A1              ", "Active Electrode, pin type                                                      ",
	    "uV      ", "-262144 ", "262144  ", "-8388608", "8388607 ",
	    "HP: DC; LP: 500 Hz                                                              ", "2048    ",
	    "Reserved                        "});
	expected_headers.push_back(duckdb::RawEDFSignalHeader {
	    "A2              ", "Active Electrode, pin type                                                      ",
	    "uV      ", "-262144 ", "262144  ", "-8388608", "8388607 ",
	    "HP: DC; LP: 500 Hz                                                              ", "2048    ",
	    "Reserved                        "});
	expected_headers.push_back(duckdb::RawEDFSignalHeader {
	    "A3              ", "Active Electrode, pin type                                                      ",
	    "uV      ", "-262144 ", "262144  ", "-8388608", "8388607 ",
	    "HP: DC; LP: 500 Hz                                                              ", "2048    ",
	    "Reserved                        "});

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	return 0;
}
