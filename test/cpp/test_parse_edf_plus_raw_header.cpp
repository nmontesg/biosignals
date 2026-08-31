#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include "duckdb/common/local_file_system.hpp"

int main(int argc, char *argv[]) {
	duckdb::LocalFileSystem fs;
	std::unique_ptr<duckdb::FileHandle> fh = fs.OpenFile(argv[1], duckdb::FileOpenFlags::FILE_FLAGS_READ);
	duckdb::RawEDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseRawGlobalHeader(*fh);

	BiosignalsTestUtils::AssertStringEquals(global_header.version, "0       ");
	BiosignalsTestUtils::AssertStringEquals(
	    global_header.patient_id, "X X X X                                                                         ");
	BiosignalsTestUtils::AssertStringEquals(
	    global_header.recording_id, "Startdate 10-DEC-2009 X X test_generator                                        ");
	BiosignalsTestUtils::AssertStringEquals(global_header.start_date, "10.12.09");
	BiosignalsTestUtils::AssertStringEquals(global_header.start_time, "12.44.02");
	BiosignalsTestUtils::AssertStringEquals(global_header.num_header_bytes, "3328    ");
	BiosignalsTestUtils::AssertStringEquals(global_header.reserved, "EDF+C                                       ");
	BiosignalsTestUtils::AssertStringEquals(global_header.num_records, "600     ");
	BiosignalsTestUtils::AssertStringEquals(global_header.duration_record, "1       ");
	BiosignalsTestUtils::AssertStringEquals(global_header.num_signals, "12  ");

	return 0;
}
