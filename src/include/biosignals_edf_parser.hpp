#pragma once

#include "duckdb.hpp" // IWYU pragma: keep
#include "duckdb/common/file_system.hpp"

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

struct EDFGlobalHeader {
	int version;
	string patient_id;
	string recording_id;

	unsigned int start_date_year;
	unsigned int start_date_month;
	unsigned int start_date_day;

	unsigned int start_time_hour;
	unsigned int start_time_minute;
	unsigned int start_time_seconds;

	size_t num_header_bytes;
	string reserved;
	int num_records;
	float duration_record;
	int num_signals;

	// bool is_plus = false;
	// bool is_discontinuous = false;
};

class BiosignalsEDFParser {
private:
	static void ParseFieldFromFile(FileHandle &, idx_t &, size_t, char *, size_t);

	template <typename T>
	static void ParseFieldFromRawHeader(const char *, T &);

	static vector<unsigned int> ValidateDateTime(const string &, bool = false);

public:
	static RawEDFGlobalHeader ParseRawGlobalHeader(FileHandle &);
	static EDFGlobalHeader ParseGlobalHeader(FileHandle &);
};

} // namespace duckdb
