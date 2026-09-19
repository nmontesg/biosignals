#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);
	duckdb::EDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseGlobalHeader(fh);

	duckdb::EDFGlobalHeader expected_header;
	expected_header.version = "0";
	expected_header.patient_id = "test file";
	expected_header.recording_id = "EDF generator";

	expected_header.start_date_year = 2008;
	expected_header.start_date_month = 10;
	expected_header.start_date_day = 2;

	expected_header.start_time_hour = 14;
	expected_header.start_time_minute = 27;
	expected_header.start_time_seconds = 0;

	expected_header.num_header_bytes = 4352;
	expected_header.reserved = "";
	expected_header.num_records = 900;
	expected_header.duration_record = 1.0;
	expected_header.num_signals = 16;

	expected_header.is_bdf = false;
	expected_header.is_plus = false;
	expected_header.is_discontinuous = false;

	BiosignalsTestUtils::CompareEDFGlobalHeader(&global_header, &expected_header);

	return 0;
}
