#pragma once

#include <iomanip>

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
	string version;
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

	bool is_bdf = false;
	bool is_plus = false;
	bool is_discontinuous = false;

	string GetFormattedStartTimestamp() {
		std::ostringstream oss;
		oss << std::setfill('0') << std::setw(4) << start_date_year << '-' << std::setw(2) << start_date_month << '-'
		    << std::setw(2) << start_date_day << ' ' << std::setw(2) << start_time_hour << ':' << std::setw(2)
		    << start_time_minute << ':' << std::setw(2) << start_time_seconds;
		return oss.str();
	}
};

class BiosignalsEDFParser {
private:
	static void ParseFieldFromFile(FileHandle &, idx_t &, size_t, char *);

	template <typename T>
	static void ParseFieldFromRawHeader(const char *, T &);

	static vector<unsigned int> ValidateDateTime(const string &, bool = false);
	static bool CheckBDFVersion(const char *);

public:
	static RawEDFGlobalHeader ParseRawGlobalHeader(FileHandle &);
	static EDFGlobalHeader ParseGlobalHeader(FileHandle &);
};

} // namespace duckdb
