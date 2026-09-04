#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/file_system.hpp"

int main(int argc, char *argv[]) {
	std::unique_ptr<duckdb::FileSystem> fs = duckdb::FileSystem::CreateLocal();
	std::unique_ptr<duckdb::FileHandle> fh = fs->OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	duckdb::RawEDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseRawGlobalHeader(*fh);
	fh->Close();

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
