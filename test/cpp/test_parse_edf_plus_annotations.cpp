#include "biosignals_edf_parser.hpp"
#include "biosignals_test_utils.hpp"
#include <cstddef>

int main(int argc, char *argv[]) {
	duckdb::DuckDB db(":memory");
	duckdb::unique_ptr<duckdb::CachingFileHandle> fh = BiosignalsTestUtils::GetCachingFileHandle(db, argv[1]);

	duckdb::EDFGlobalHeader global_header = duckdb::BiosignalsEDFParser::ParseGlobalHeader(fh);
	duckdb::vector<duckdb::EDFSignalHeader> signal_headers =
	    duckdb::BiosignalsEDFParser::ParseSignalHeaders(fh, global_header);

	// the last signal is an annotation signal
	int num_signals = global_header.num_signals;
	BiosignalsTestUtils::AssertEquals<bool>(signal_headers[num_signals - 1].is_annotation, true);

	idx_t record_idx = 0;

	size_t header_size = duckdb::BiosignalsEDFParser::GetTotalHeaderSize(num_signals);
	size_t record_size = duckdb::BiosignalsEDFParser::GetRecordSize<2>(signal_headers);
	idx_t record_offset = header_size + record_idx * record_size;
	idx_t signal_offset = duckdb::BiosignalsEDFParser::GetSignalOffsetWithinRecord<2>(num_signals - 1, signal_headers);
	idx_t offset = record_offset + signal_offset;
	size_t num_bytes_to_copy = static_cast<size_t>(signal_headers[num_signals - 1].samples_per_record * 2);

	std::vector<unsigned char> buffer;
	buffer.resize(num_bytes_to_copy);
	
	duckdb::shared_ptr<duckdb::FileHandle> shared_fh = fh->GetFileHandle();
	shared_fh->Read(buffer.data(), num_bytes_to_copy, offset);

	std::string annotation_data(buffer.begin(), buffer.end());
	duckdb::vector<duckdb::TimestampedAnnotationList> tals =
	    duckdb::BiosignalsEDFParser::ParseRecordTALs(annotation_data);

	BiosignalsTestUtils::AssertEquals<float>(tals[0].onset, 0);

	BiosignalsTestUtils::AssertEquals<float>(tals[1].onset, 0);
	BiosignalsTestUtils::AssertEquals<bool>(tals[1].has_duration, true);
	BiosignalsTestUtils::AssertEquals<float>(tals[1].duration, 30630.);
	BiosignalsTestUtils::AssertEquals<size_t>(tals[1].annotations.size(), 1);
	BiosignalsTestUtils::AssertStringEquals(tals[1].annotations[0], "Sleep stage W");

	BiosignalsTestUtils::AssertEquals<float>(tals[2].onset, 30630.);
	BiosignalsTestUtils::AssertEquals<bool>(tals[2].has_duration, true);
	BiosignalsTestUtils::AssertEquals<float>(tals[2].duration, 120.);
	BiosignalsTestUtils::AssertEquals<size_t>(tals[2].annotations.size(), 1);
	BiosignalsTestUtils::AssertStringEquals(tals[2].annotations[0], "Sleep stage 1");

	return 0;
}
