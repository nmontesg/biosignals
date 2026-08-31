#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/local_file_system.hpp"

int main(int argc, char *argv[]) {
	duckdb::LocalFileSystem fs;
	std::unique_ptr<duckdb::FileHandle> fh = fs.OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	duckdb::EDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseGlobalHeader(*fh);

	BiosignalsTestUtils::AssertStringEquals(global_header.version, "BIOSEMI");
	BiosignalsTestUtils::AssertStringEquals(global_header.patient_id, "");
	BiosignalsTestUtils::AssertStringEquals(global_header.recording_id, "");

	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_date_year, 2005);
	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_date_month, 11);
	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_date_day, 1);

	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_time_hour, 19);
	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_time_minute, 37);
	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_time_seconds, 16);

	BiosignalsTestUtils::AssertEquals<size_t>(global_header.num_header_bytes, 4608);
	BiosignalsTestUtils::AssertStringEquals(global_header.reserved, "24BIT");
	BiosignalsTestUtils::AssertEquals<int>(global_header.num_records, 60);
	BiosignalsTestUtils::AssertEquals<float>(global_header.duration_record, 1.0);
	BiosignalsTestUtils::AssertEquals<int>(global_header.num_signals, 17);

	BiosignalsTestUtils::AssertEquals<bool>(global_header.is_bdf, true);
	BiosignalsTestUtils::AssertEquals<bool>(global_header.is_plus, false);
	BiosignalsTestUtils::AssertEquals<bool>(global_header.is_discontinuous, false);

	return 0;
}
