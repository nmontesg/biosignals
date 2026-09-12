#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	std::vector<duckdb::RawEDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseRawSignalHeaders(fh);

	std::vector<duckdb::RawEDFSignalHeader> expected_headers;
	expected_headers.reserve(3);
	expected_headers.push_back(duckdb::RawEDFSignalHeader {
	    "F4              ", "AgAgCl                                                                          ",
	    "uV      ", "-3200   ", "3200    ", "-32768  ", "32767   ",
	    "HP:0.015Hz                                                                      ", "200     ",
	    "                                "});
	expected_headers.push_back(duckdb::RawEDFSignalHeader {
	    "F3              ", "AgAgCl                                                                          ",
	    "uV      ", "-3200   ", "3200    ", "-32768  ", "32767   ",
	    "HP:0.015Hz                                                                      ", "100     ",
	    "                                "});
	expected_headers.push_back(duckdb::RawEDFSignalHeader {
	    "X10             ", "AgAgCl                                                                          ",
	    "mV      ", "-1.6    ", "1.6     ", "-32768  ", "32767   ",
	    "HP:0.015Hz                                                                      ", "200     ",
	    "                                "});

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	return 0;
}
