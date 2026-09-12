#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	duckdb::RawEDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseRawGlobalHeader(fh);

	duckdb::RawEDFGlobalHeader expected_hdr = {
	    {static_cast<char>(0xFF), 0x42, 0x49, 0x4F, 0x53, 0x45, 0x4D, 0x49, '\0'},
	    "X X X X                                                                         ",
	    "Startdate 10-DEC-2009 X X test_generator                                        ",
	    "10.12.09",
	    "12.45.54",
	    "3328    ",
	    "BDF+C                                       ",
	    "600     ",
	    "1       ",
	    "12  "};

	BiosignalsTestUtils::CompareRawEDFGlobalHeader(&global_header, &expected_hdr, true);

	return 0;
}
