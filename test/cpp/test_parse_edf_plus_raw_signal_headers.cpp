#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	std::vector<duckdb::RawEDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseRawSignalHeaders(fh);

	std::vector<duckdb::RawEDFSignalHeader> expected_headers;
	expected_headers.reserve(2);
	expected_headers.push_back(duckdb::RawEDFSignalHeader {
	    "squarewave      ", "                                                                                ",
	    "uV      ", "-1000.00", "1000.000", "-32768  ", "32767   ",
	    "                                                                                ", "200     ",
	    "                                "});
	expected_headers.push_back(duckdb::RawEDFSignalHeader {
	    "ramp            ", "                                                                                ",
	    "uV      ", "-1000.00", "1000.000", "-32768  ", "32767   ",
	    "                                                                                ", "200     ",
	    "                                "});

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	// check the last header for the annotation signal
	duckdb::RawEDFSignalHeader expected_annotation_signal_hdr = {
	    "EDF Annotations ", "                                                                                ",
	    "        ",         "-1      ",
	    "1       ",         "-32768  ",
	    "32767   ",         "                                                                                ",
	    "51      ",         "                                "};
	size_t num_headers = headers.size();
	BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[num_headers - 1], &expected_annotation_signal_hdr);

	return 0;
}
