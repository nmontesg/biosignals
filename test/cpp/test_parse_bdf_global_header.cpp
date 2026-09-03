#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/local_file_system.hpp"

int main(int argc, char *argv[]) {
	duckdb::LocalFileSystem fs;
	std::unique_ptr<duckdb::FileHandle> fh = fs.OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	duckdb::EDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseGlobalHeader(*fh);

	duckdb::EDFGlobalHeader expected_header;
	expected_header.version = "BIOSEMI";
	expected_header.patient_id = "";
	expected_header.recording_id = "";

	expected_header.start_date_year = 2005;
	expected_header.start_date_month = 11;
	expected_header.start_date_day = 1;

	expected_header.start_time_hour = 19;
	expected_header.start_time_minute = 37;
	expected_header.start_time_seconds = 16;

	expected_header.num_header_bytes = 4608;
	expected_header.reserved = "24BIT";
	expected_header.num_records = 60;
	expected_header.duration_record = 1.0;
	expected_header.num_signals = 17;

	expected_header.is_bdf = true;
	expected_header.is_plus = false;
	expected_header.is_discontinuous = false;

	BiosignalsTestUtils::CompareEDFGlobalHeader(&global_header, &expected_header);

	return 0;
}
