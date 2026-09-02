#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/local_file_system.hpp"

int main(int argc, char *argv[]) {
	duckdb::LocalFileSystem fs;
	std::unique_ptr<duckdb::FileHandle> fh = fs.OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	std::vector<duckdb::RawEDFSignalHeader> headers = duckdb::BiosignalsEDFParser::ParseRawSignalHeaders(*fh);

	// TODO remove;
	for (auto signal_hdr : headers) {
		std::cout << signal_hdr.label << ',' << '\n';
		std::cout << signal_hdr.transducer_type << ',' << '\n';
		std::cout << signal_hdr.physical_dimension << ',' << '\n';
		std::cout << signal_hdr.physical_min << ',' << '\n';
		std::cout << signal_hdr.physical_max << ',' << '\n';
		std::cout << signal_hdr.digital_min << ',' << '\n';
		std::cout << signal_hdr.digital_max << ',' << '\n';
		std::cout << signal_hdr.prefiltering << ',' << '\n';
		std::cout << signal_hdr.samples_per_record << ',' << '\n';
		std::cout << signal_hdr.reserved << ',' << '\n';
		std::cout << '\n';
	}

	std::vector<duckdb::RawEDFSignalHeader> expected_headers = {
	    {"squarewave      ", "                                                                                ",
	     "uV      ", "-1000.00", "1000.000", "-32768  ", "32767   ",
	     "                                                                                ", "200     ",
	     "                                "},
	    {"ramp            ", "                                                                                ",
	     "uV      ", "-1000.00", "1000.000", "-32768  ", "32767   ",
	     "                                                                                ", "200     ",
	     "                                "},
	};

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

	// check the last header for the annotation signal
	duckdb::RawEDFSignalHeader expected_annotation_signal_hdr = {
	    "EDF Annotations ", "                                                                                ",
	    "        ",         "-1      ",
	    "1       ",         "-32768  ",
	    "32767   ",         "                                                                                ",
	    "51      ",         "                                "};
	size_t num_headers = headers.size();
	BiosignalsTestUtils::AssertStringEquals(headers[num_headers - 1].label, expected_annotation_signal_hdr.label);
	BiosignalsTestUtils::AssertStringEquals(headers[num_headers - 1].transducer_type,
	                                        expected_annotation_signal_hdr.transducer_type);
	BiosignalsTestUtils::AssertStringEquals(headers[num_headers - 1].physical_dimension,
	                                        expected_annotation_signal_hdr.physical_dimension);
	BiosignalsTestUtils::AssertStringEquals(headers[num_headers - 1].physical_min,
	                                        expected_annotation_signal_hdr.physical_min);
	BiosignalsTestUtils::AssertStringEquals(headers[num_headers - 1].physical_max,
	                                        expected_annotation_signal_hdr.physical_max);
	BiosignalsTestUtils::AssertStringEquals(headers[num_headers - 1].prefiltering,
	                                        expected_annotation_signal_hdr.prefiltering);
	BiosignalsTestUtils::AssertStringEquals(headers[num_headers - 1].samples_per_record,
	                                        expected_annotation_signal_hdr.samples_per_record);
	BiosignalsTestUtils::AssertStringEquals(headers[num_headers - 1].reserved, expected_annotation_signal_hdr.reserved);

	return 0;
}
