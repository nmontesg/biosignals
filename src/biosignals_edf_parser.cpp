#include "biosignals_edf_parser.hpp"
#include "duckdb/common/file_system.hpp"

namespace duckdb {

// pair<const char *, size_t> BiosignalsEDFParser::RemoveTrailingSpaces(const char *data, size_t size) {
// 	if (!data || size == 0) {
// 		return {data, 0};
// 	}

// 	size_t end = size;
// 	while (end > 0 && data[end - 1] == ' ') {
// 		--end;
// 	}

// 	return {data, end};
// }

void BiosignalsEDFParser::ParseField(FileHandle &fh, idx_t &offset, size_t num_bytes, char *buffer) {
	fh.Seek(offset);
	fh.Read(buffer, num_bytes);
	buffer[num_bytes] = '\0';
	offset += num_bytes;
}

RawEDFGlobalHeader BiosignalsEDFParser::ParseRawGlobalHeader(FileHandle &fh) {
	RawEDFGlobalHeader raw_header;
	idx_t offset = 0;

	BiosignalsEDFParser::ParseField(fh, offset, 8, raw_header.version);
	BiosignalsEDFParser::ParseField(fh, offset, 80, raw_header.patient_id);
	BiosignalsEDFParser::ParseField(fh, offset, 80, raw_header.recording_id);
	BiosignalsEDFParser::ParseField(fh, offset, 8, raw_header.start_date);
	BiosignalsEDFParser::ParseField(fh, offset, 8, raw_header.start_time);
	BiosignalsEDFParser::ParseField(fh, offset, 8, raw_header.num_header_bytes);
	BiosignalsEDFParser::ParseField(fh, offset, 44, raw_header.reserved);
	BiosignalsEDFParser::ParseField(fh, offset, 8, raw_header.num_records);
	BiosignalsEDFParser::ParseField(fh, offset, 8, raw_header.duration_record);
	BiosignalsEDFParser::ParseField(fh, offset, 4, raw_header.num_signals);

	return raw_header;
}

} // namespace duckdb
