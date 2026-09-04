#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/file_system.hpp"

int main(int argc, char *argv[]) {
	std::unique_ptr<duckdb::FileSystem> fs = duckdb::FileSystem::CreateLocal();
	std::unique_ptr<duckdb::FileHandle> fh = fs->OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	duckdb::EDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseGlobalHeader(*fh);
	fh->Close();

	duckdb::EDFGlobalHeader expected_header;
	expected_header.version = "BIOSEMI";
	expected_header.patient_id = "X X X X";
	expected_header.recording_id = "Startdate 10-DEC-2009 X X test_generator";

	expected_header.start_date_year = 2010;
	expected_header.start_date_month = 12;
	expected_header.start_date_day = 9;

	expected_header.start_time_hour = 12;
	expected_header.start_time_minute = 45;
	expected_header.start_time_seconds = 54;

	expected_header.num_header_bytes = 3328;
	expected_header.reserved = "BDF+C";
	expected_header.num_records = 600;
	expected_header.duration_record = 1.0;
	expected_header.num_signals = 12;

	expected_header.is_bdf = true;
	expected_header.is_plus = true;
	expected_header.is_discontinuous = false;

	BiosignalsTestUtils::CompareEDFGlobalHeader(&global_header, &expected_header);

	return 0;
}
