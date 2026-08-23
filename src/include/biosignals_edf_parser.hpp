#pragma once

#include "duckdb.hpp" // IWYU pragma: keep

namespace duckdb {

struct RawEDFGlobalHeader {
	char version[8];
	char patient_id[80];
	char recording_id[80];
	char start_date[8];
	char start_time[8];
	char num_header_bytes[8];
	char reserved[44];
	char num_records[8];
	char duration_record[8];
	char num_signals[4];
};

class BiosignalsEDFParser {
private:
	static pair<const char *, size_t> RemoveTrailingSpaces(const char *data, size_t size);

public:
	static RawEDFGlobalHeader ParseRawGlobalHeader(FileHandle &);
};

} // namespace duckdb
