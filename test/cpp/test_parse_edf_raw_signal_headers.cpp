#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/file_system.hpp"

int main(int argc, char *argv[]) {
	std::unique_ptr<duckdb::FileSystem> fs = duckdb::FileSystem::CreateLocal();
	std::unique_ptr<duckdb::FileHandle> fh = fs->OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	std::vector<duckdb::RawEDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseRawSignalHeaders(*fh);
	fh->Close();

	std::vector<duckdb::RawEDFSignalHeader> expected_headers;
	expected_headers.reserve(3);
	expected_headers.push_back({"F4              ",
	                            "AgAgCl                                                                          ",
	                            "uV      ", "-3200   ", "3200    ", "-32768  ", "32767   ",
	                            "HP:0.015Hz                                                                      ",
	                            "200     ", "                                "});
	expected_headers.push_back({"F3              ",
	                            "AgAgCl                                                                          ",
	                            "uV      ", "-3200   ", "3200    ", "-32768  ", "32767   ",
	                            "HP:0.015Hz                                                                      ",
	                            "100     ", "                                "});
	expected_headers.push_back({"X10             ",
	                            "AgAgCl                                                                          ",
	                            "mV      ", "-1.6    ", "1.6     ", "-32768  ", "32767   ",
	                            "HP:0.015Hz                                                                      ",
	                            "200     ", "                                "});

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	return 0;
}
