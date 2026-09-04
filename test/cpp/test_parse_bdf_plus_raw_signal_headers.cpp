#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/file_system.hpp"

int main(int argc, char *argv[]) {
	std::unique_ptr<duckdb::FileSystem> fs = duckdb::FileSystem::CreateLocal();
	std::unique_ptr<duckdb::FileHandle> fh = fs->OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	std::vector<duckdb::RawEDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseRawSignalHeaders(*fh);
	fh->Close();

	std::vector<duckdb::RawEDFSignalHeader> expected_headers;
	expected_headers.reserve(2);
	expected_headers.push_back({"squarewave      ",
	                            "                                                                                ",
	                            "uV      ", "-1000.00", "1000.000", "-8388608", "8388607 ",
	                            "                                                                                ",
	                            "200     ", "                                "});
	expected_headers.push_back({"ramp            ",
	                            "                                                                                ",
	                            "uV      ", "-1000.00", "1000.000", "-8388608", "8388607 ",
	                            "                                                                                ",
	                            "200     ", "                                "});

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	// check the last header for the annotation signal
	duckdb::RawEDFSignalHeader expected_annotation_signal_hdr = {
	    "BDF Annotations ", "                                                                                ",
	    "        ",         "-1      ",
	    "1       ",         "-8388608",
	    "8388607 ",         "                                                                                ",
	    "34      ",         "                                "};
	size_t num_headers = headers.size();
	BiosignalsTestUtils::CompareRawEDFSignalHeader(&headers[num_headers - 1], &expected_annotation_signal_hdr);

	return 0;
}
