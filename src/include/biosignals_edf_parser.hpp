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

	bool operator==(const RawEDFGlobalHeader &rhs) const {
		return (strcmp(version, rhs.version) == 0) && (strcmp(patient_id, rhs.patient_id) == 0) &&
		       (strcmp(recording_id, rhs.recording_id) == 0) && (strcmp(start_date, rhs.start_date) == 0) &&
		       (strcmp(start_time, rhs.start_time) == 0) && (strcmp(num_header_bytes, rhs.num_header_bytes) == 0) &&
		       (strcmp(reserved, rhs.reserved) == 0) && (strcmp(num_records, rhs.num_records) == 0) &&
		       (strcmp(duration_record, rhs.duration_record) == 0) && (strcmp(num_signals, rhs.num_signals) == 0);
	};
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

	bool operator==(const EDFGlobalHeader &rhs) const {
		return (version == rhs.version) && (patient_id == rhs.patient_id) && (recording_id == rhs.recording_id) &&
		       (start_date_year == rhs.start_date_year) && (start_date_month == rhs.start_date_month) &&
		       (start_date_day == rhs.start_date_day) && (start_time_hour == rhs.start_time_hour) &&
		       (start_time_minute == rhs.start_time_minute) && (start_time_seconds == rhs.start_time_seconds) &&
		       (num_header_bytes == rhs.num_header_bytes) && (reserved == rhs.reserved) &&
		       (num_records == rhs.num_records) && (duration_record == rhs.duration_record) &&
		       (num_signals == rhs.num_signals) && (is_bdf == rhs.is_bdf) && (is_plus == rhs.is_plus) &&
		       (is_discontinuous == rhs.is_discontinuous);
	};

	string GetFormattedStartTimestamp() {
		std::ostringstream oss;
		oss << std::setfill('0') << std::setw(4) << start_date_year << '-' << std::setw(2) << start_date_month << '-'
		    << std::setw(2) << start_date_day << ' ' << std::setw(2) << start_time_hour << ':' << std::setw(2)
		    << start_time_minute << ':' << std::setw(2) << start_time_seconds;
		return oss.str();
	}
};

struct RawEDFSignalHeader {
	char label[17];
	char transducer_type[81];
	char physical_dimension[9];
	char physical_min[9];
	char physical_max[9];
	char digital_min[9];
	char digital_max[9];
	char prefiltering[81];
	char samples_per_record[9];
	char reserved[33];

	bool operator==(const RawEDFSignalHeader &rhs) const {
		return (strcmp(label, rhs.label) == 0) && (strcmp(transducer_type, rhs.transducer_type) == 0) &&
		       (strcmp(physical_dimension, rhs.physical_dimension) == 0) &&
		       (strcmp(physical_min, rhs.physical_min) == 0) && (strcmp(physical_max, rhs.physical_max) == 0) &&
		       (strcmp(digital_min, rhs.digital_min) == 0) && (strcmp(digital_max, rhs.digital_max) == 0) &&
		       (strcmp(prefiltering, rhs.prefiltering) == 0) &&
		       (strcmp(samples_per_record, rhs.samples_per_record) == 0) && (strcmp(reserved, rhs.reserved) == 0);
	};
};

#define PARSE_EDF_FIELD(FH, OFFSET, NUM_SIGNALS, SIGNAL_HEADERS, FIELD)                                                \
	for (idx_t i = 0; i < NUM_SIGNALS; i++) {                                                                          \
		BiosignalsEDFParser::ParseFieldFromFile(FH, OFFSET, sizeof(RawEDFSignalHeader::FIELD) - 1,                     \
		                                        SIGNAL_HEADERS[i].FIELD);                                              \
	}

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

	static vector<RawEDFSignalHeader> ParseRawSignalHeaders(FileHandle &);
};

} // namespace duckdb
