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
		BiosignalsTestUtils::AssertStringEquals(headers[i].label, expected_headers[i].label);
		BiosignalsTestUtils::AssertStringEquals(headers[i].transducer_type, expected_headers[i].transducer_type);
		BiosignalsTestUtils::AssertStringEquals(headers[i].physical_dimension, expected_headers[i].physical_dimension);
		BiosignalsTestUtils::AssertStringEquals(headers[i].physical_min, expected_headers[i].physical_min);
		BiosignalsTestUtils::AssertStringEquals(headers[i].physical_max, expected_headers[i].physical_max);
		BiosignalsTestUtils::AssertStringEquals(headers[i].prefiltering, expected_headers[i].prefiltering);
		BiosignalsTestUtils::AssertStringEquals(headers[i].samples_per_record, expected_headers[i].samples_per_record);
		BiosignalsTestUtils::AssertStringEquals(headers[i].reserved, expected_headers[i].reserved);
	}

	return 0;
}
