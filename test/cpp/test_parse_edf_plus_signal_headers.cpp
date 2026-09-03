#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/file_system.hpp"
#include <vector>

int main(int argc, char *argv[]) {
	std::unique_ptr<duckdb::FileSystem> fs = duckdb::FileSystem::CreateLocal();
	std::unique_ptr<duckdb::FileHandle> fh = fs->OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	std::vector<duckdb::EDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseSignalHeaders(*fh);

	std::vector<duckdb::EDFSignalHeader> expected_headers = {
	    duckdb::EDFSignalHeader("squarewave", "", "uV", -1000.0, 1000.0, -32768, 32767, "", 200, "", false),
	    duckdb::EDFSignalHeader("ramp", "", "uV", -1000.0, 1000.0, -32768, 32767, "", 200, "", false),
	};

	size_t num_hdrs_check = expected_headers.size();
	for (idx_t i = 0; i < num_hdrs_check; i++) {
		BiosignalsTestUtils::CompareEDFSignalHeader(&headers[i], &expected_headers[i]);
	}

	// check the last header for the annotation signal
	duckdb::EDFSignalHeader expected_annotation_signal_hdr("EDF Annotations", "", "", -1.0, 1.0, -32768, 32767, "", 51,
	                                                       "", true);
	size_t num_headers = headers.size();
	BiosignalsTestUtils::CompareEDFSignalHeader(&headers[num_headers - 1], &expected_annotation_signal_hdr);

	return 0;
}
