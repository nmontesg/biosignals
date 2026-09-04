#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/file_system.hpp"

int main(int argc, char *argv[]) {
	std::unique_ptr<duckdb::FileSystem> fs = duckdb::FileSystem::CreateLocal();
	std::unique_ptr<duckdb::FileHandle> fh = fs->OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	std::vector<duckdb::RawEDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseRawSignalHeaders(*fh);

	std::vector<duckdb::RawEDFSignalHeader> expected_headers;
	expected_headers.reserve(3);
	expected_headers.push_back({"A1              ",
	                            "Active Electrode, pin type                                                      ",
	                            "uV      ", "-262144 ", "262144  ", "-8388608", "8388607 ",
	                            "HP: DC; LP: 500 Hz                                                              ",
	                            "2048    ", "Reserved                        "});
	expected_headers.push_back({"A2              ",
	                            "Active Electrode, pin type                                                      ",
	                            "uV      ", "-262144 ", "262144  ", "-8388608", "8388607 ",
	                            "HP: DC; LP: 500 Hz                                                              ",
	                            "2048    ", "Reserved                        "});
	expected_headers.push_back({"A3              ",
	                            "Active Electrode, pin type                                                      ",
	                            "uV      ", "-262144 ", "262144  ", "-8388608", "8388607 ",
	                            "HP: DC; LP: 500 Hz                                                              ",
	                            "2048    ", "Reserved                        "});

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	return 0;
}
