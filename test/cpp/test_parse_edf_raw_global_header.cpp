#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	duckdb::RawEDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseRawGlobalHeader(fh);

	duckdb::RawEDFGlobalHeader expected_hdr = {
	    "0       ",
	    "test file                                                                       ",
	    "EDF generator                                                                   ",
	    "02.10.08",
	    "14.27.00",
	    "4352    ",
	    "                                            ",
	    "900     ",
	    "1       ",
	    "16  "};

	// even though we could run the comparison operator, use the utils functions to get more
	// information about when the failure happens (if it happens)
	BiosignalsTestUtils::CompareRawEDFGlobalHeader(&global_header, &expected_hdr);

	return 0;
}
