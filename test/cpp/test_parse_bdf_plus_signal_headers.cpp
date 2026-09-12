#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	std::vector<duckdb::EDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseSignalHeaders(fh);

	std::vector<duckdb::EDFSignalHeader> expected_headers = {
	    duckdb::EDFSignalHeader("squarewave", "", "uV", -1000.0, 1000.0, -8388608, 8388607, "", 200, "", false),
	    duckdb::EDFSignalHeader("ramp", "", "uV", -1000.0, 1000.0, -8388608, 8388607, "", 200, "", false)};

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	// check the last header for the annotation signal
	duckdb::EDFSignalHeader expected_annotation_signal_hdr("BDF Annotations", "", "", -1.0, 1.0, -8388608, 8388607, "",
	                                                       34, "", true);
	size_t num_headers = headers.size();
	BiosignalsTestUtils::CompareEDFSignalHeader(&headers[num_headers - 1], &expected_annotation_signal_hdr);

	return 0;
}
