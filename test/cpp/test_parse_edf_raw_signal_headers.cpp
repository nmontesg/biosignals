#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/local_file_system.hpp"

int main(int argc, char *argv[]) {
	duckdb::LocalFileSystem fs;
	std::unique_ptr<duckdb::FileHandle> fh = fs.OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	std::vector<duckdb::RawEDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseRawSignalHeaders(*fh);

	std::vector<duckdb::RawEDFSignalHeader> expected_headers = {
	    {"F4              ", "AgAgCl                                                                          ",
	     "uV      ", "-3200   ", "3200    ", "-32768  ", "32767   ",
	     "HP:0.015Hz                                                                      ", "200     ",
	     "                                "},
	    {"F3              ", "AgAgCl                                                                          ",
	     "uV      ", "-3200   ", "3200    ", "-32768  ", "32767   ",
	     "HP:0.015Hz                                                                      ", "100     ",
	     "                                "},
	    {"X10             ", "AgAgCl                                                                          ",
	     "mV      ", "-1.6    ", "1.6     ", "-32768  ", "32767   ",
	     "HP:0.015Hz                                                                      ", "200     ",
	     "                                "}};

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	return 0;
}
