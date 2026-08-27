#pragma once

#include "duckdb.hpp" // IWYU pragma: keep

namespace duckdb {

struct RawEDFGlobalHeader {
	char version[9];
	char patient_id[81];
	char recording_id[81];
	char start_date[9];
	char start_time[9];
	char num_header_bytes[9];
	char reserved[45];
	char num_records[9];
	char duration_record[9];
	char num_signals[5];
};

class BiosignalsEDFParser {
private:
	static void ParseField(FileHandle &, idx_t &, size_t, char *);
	// static pair<const char *, size_t> RemoveTrailingSpaces(const char *data, size_t size);

public:
	static RawEDFGlobalHeader ParseRawGlobalHeader(FileHandle &);
};

} // namespace duckdb
