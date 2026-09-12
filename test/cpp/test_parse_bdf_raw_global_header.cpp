#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	duckdb::RawEDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseRawGlobalHeader(fh);

	duckdb::RawEDFGlobalHeader expected_hdr = {
	    {static_cast<char>(0xFF), 0x42, 0x49, 0x4F, 0x53, 0x45, 0x4D, 0x49, '\0'},
	    "                                                                                ",
	    "                                                                                ",
	    "05.11.01",
	    "19.37.16",
	    "4608    ",
	    "24BIT                                       ",
	    "60      ",
	    "1       ",
	    "17  "};

	BiosignalsTestUtils::CompareRawEDFGlobalHeader(&global_header, &expected_hdr, true);

	return 0;
}
