#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/local_file_system.hpp"
#include "test_biosignals_edf_parser.hpp"

void TestBiosignalsEDFParser::TestParseRawGlobalHeader(const char *filepath) {
	duckdb::LocalFileSystem fs;
	std::unique_ptr<duckdb::FileHandle> fh = fs.OpenFile(filepath, duckdb::FileOpenFlags::FILE_FLAGS_READ);
	duckdb::RawEDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseRawGlobalHeader(*fh);

	BiosignalsTestUtils::AssertStringEquals(global_header.version, "0       ");
	BiosignalsTestUtils::AssertStringEquals(
	    global_header.patient_id, "test file                                                                       ");
	BiosignalsTestUtils::AssertStringEquals(
	    global_header.recording_id, "EDF generator                                                                   ");
	BiosignalsTestUtils::AssertStringEquals(global_header.start_date, "02.10.08");
	BiosignalsTestUtils::AssertStringEquals(global_header.start_time, "14.27.00");
	BiosignalsTestUtils::AssertStringEquals(global_header.num_header_bytes, "4352    ");
	BiosignalsTestUtils::AssertStringEquals(global_header.reserved, "                                            ");
	BiosignalsTestUtils::AssertStringEquals(global_header.num_records, "900     ");
	BiosignalsTestUtils::AssertStringEquals(global_header.duration_record, "1       ");
}

void TestBiosignalsEDFParser::TestParseGlobalHeader(const char *filepath) {
	duckdb::LocalFileSystem fs;
	std::unique_ptr<duckdb::FileHandle> fh = fs.OpenFile(filepath, duckdb::FileOpenFlags::FILE_FLAGS_READ);
	duckdb::EDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseGlobalHeader(*fh);

	BiosignalsTestUtils::AssertEquals<int>(global_header.version, 0);
	BiosignalsTestUtils::AssertStringEquals(global_header.patient_id, "test file");
	BiosignalsTestUtils::AssertStringEquals(global_header.recording_id, "EDF generator");

	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_date_year, 2002);
	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_date_month, 10);
	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_date_day, 8);

	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_time_hour, 14);
	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_time_minute, 27);
	BiosignalsTestUtils::AssertEquals<unsigned int>(global_header.start_time_seconds, 0);

	BiosignalsTestUtils::AssertEquals<size_t>(global_header.num_header_bytes, 4352);
	BiosignalsTestUtils::AssertStringEquals(global_header.reserved, "");
	BiosignalsTestUtils::AssertEquals<int>(global_header.num_records, 900);
	BiosignalsTestUtils::AssertEquals<float>(global_header.duration_record, 1.0);
}
