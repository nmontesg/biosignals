#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/file_system.hpp"

int main(int argc, char *argv[]) {
	std::unique_ptr<duckdb::FileSystem> fs = duckdb::FileSystem::CreateLocal();
	std::unique_ptr<duckdb::FileHandle> fh = fs->OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	duckdb::RawEDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseRawGlobalHeader(*fh);
	fh->Close();

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
