#pragma once

#include "biosignals_win32.hpp"

#include <cstddef>
#include <iomanip>
#include <numeric>

#include "boost/algorithm/string/trim.hpp"
#include "boost/lexical_cast.hpp"
#include "duckdb.hpp" // IWYU pragma: keep
#include "duckdb/common/unique_ptr.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/storage/external_file_cache/caching_file_system.hpp"

#define GLOBAL_HEADER_SIZE 256
#define SIGNAL_HEADER_SIZE 256

#define EDF_PLUS_CONTINUOUS    "EDF+C"
#define EDF_PLUS_DISCONTINUOUS "EDF+D"
#define EDF_ANNOTATIONS        "EDF Annotations"

#define BDF_PLUS_CONTINUOUS    "BDF+C"
#define BDF_PLUS_DISCONTINUOUS "BDF+D"
#define BDF_ANNOTATIONS        "BDF Annotations"

#define TAL_DELIMITER            0x00
#define TAL_DURATION_DELIMITER   0x15 // 21
#define TAL_ANNOTATION_DELIMITER 0x14 // 20

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
	char physical_dim[9];
	char physical_min[9];
	char physical_max[9];
	char digital_min[9];
	char digital_max[9];
	char prefiltering[81];
	char samples_per_record[9];
	char reserved[33];

	bool operator==(const RawEDFSignalHeader &rhs) const {
		return (strcmp(label, rhs.label) == 0) && (strcmp(transducer_type, rhs.transducer_type) == 0) &&
		       (strcmp(physical_dim, rhs.physical_dim) == 0) && (strcmp(physical_min, rhs.physical_min) == 0) &&
		       (strcmp(physical_max, rhs.physical_max) == 0) && (strcmp(digital_min, rhs.digital_min) == 0) &&
		       (strcmp(digital_max, rhs.digital_max) == 0) && (strcmp(prefiltering, rhs.prefiltering) == 0) &&
		       (strcmp(samples_per_record, rhs.samples_per_record) == 0) && (strcmp(reserved, rhs.reserved) == 0);
	};
};

#define PARSE_EDF_FIELD(BUF, OFFSET, NUM_SIGNALS, SIGNAL_HEADERS, FIELD)                                               \
	for (idx_t i = 0; i < NUM_SIGNALS; i++) {                                                                          \
		BiosignalsEDFParser::CopyFromBuffer(BUF, OFFSET, sizeof(RawEDFSignalHeader::FIELD) - 1,                        \
		                                    SIGNAL_HEADERS[i].FIELD);                                                  \
	}

struct EDFSignalHeader {
	string label;
	string transducer_type;

	string physical_dim;
	float physical_min;
	float physical_max;

	int digital_min;
	int digital_max;

	string prefiltering;
	unsigned int samples_per_record;
	string reserved;

	bool is_annotation = false;

	// derived fields
	float slope;
	float offset;

	void ComputeSlopeAndOffset() {
		if (digital_max == digital_min) {
			slope = 0.0;
			offset = physical_min;
		} else {
			slope = (physical_max - physical_min) / (float)(digital_max - digital_min);
			offset = physical_min - slope * (float)digital_min;
		}
	};

	explicit EDFSignalHeader() {};

	explicit EDFSignalHeader(const string &_lab, const string &_tt, const string &_physdim, float _pmin, float _pmax,
	                         int _dmin, int _dmax, const string &_prefil, unsigned int _spr, const string &_res,
	                         bool _is_ann)
	    : label(_lab), transducer_type(_tt), physical_dim(_physdim), physical_min(_pmin), physical_max(_pmax),
	      digital_min(_dmin), digital_max(_dmax), prefiltering(_prefil), samples_per_record(_spr), reserved(_res),
	      is_annotation(_is_ann) {
		ComputeSlopeAndOffset();
	};

	bool operator==(const EDFSignalHeader &rhs) const {
		return (label == rhs.label) && (transducer_type == rhs.transducer_type) && (physical_dim == rhs.physical_dim) &&
		       (physical_min == rhs.physical_min) && (physical_max == rhs.physical_max) &&
		       (digital_min == rhs.digital_max) && (prefiltering == rhs.prefiltering) &&
		       (samples_per_record == rhs.samples_per_record) && (reserved == rhs.reserved) &&
		       (is_annotation == rhs.is_annotation);
	}
};

struct TimestampedAnnotationList {
	float onset;

	bool has_duration;
	float duration;

	std::vector<std::string> annotations;
};

class BIOSIGNALS_EXPORT BiosignalsEDFParser {
private:
	static void CopyFromBuffer(char *, idx_t &, size_t, char *);

	template <typename T>
	static void ParseFieldFromBuffer(const char *buff, T &dest) {
		string raw_field_trimmed = boost::algorithm::trim_right_copy_if(string(buff), boost::is_space());
		try {
			dest = boost::lexical_cast<T>(raw_field_trimmed);
		} catch (const boost::bad_lexical_cast &) {
			throw IOException("Invalid EDF header field: could not parse \"" + raw_field_trimmed + "\"");
		}
	}

	static vector<unsigned int> ValidateDateTime(const string &, bool = false);
	static bool CheckBDFVersion(const char *);

public:
	static RawEDFGlobalHeader ParseRawGlobalHeader(unique_ptr<CachingFileHandle> &);
	static EDFGlobalHeader ParseGlobalHeader(unique_ptr<CachingFileHandle> &);

	static vector<RawEDFSignalHeader> ParseRawSignalHeaders(unique_ptr<CachingFileHandle> &, const EDFGlobalHeader &);
	static vector<RawEDFSignalHeader> ParseRawSignalHeaders(unique_ptr<CachingFileHandle> &);

	static vector<EDFSignalHeader> ParseSignalHeaders(unique_ptr<CachingFileHandle> &, const EDFGlobalHeader &);
	static vector<EDFSignalHeader> ParseSignalHeaders(unique_ptr<CachingFileHandle> &);

	static size_t GetTotalHeaderSize(const size_t);

	template <uint8_t S>
	static size_t GetRecordSize(const vector<EDFSignalHeader> &signal_hdrs) {
		size_t num_samples_per_record =
		    std::accumulate(signal_hdrs.begin(), signal_hdrs.end(), 0,
		                    [](size_t sum, const EDFSignalHeader &curr) { return sum + curr.samples_per_record; });
		return num_samples_per_record * S;
	}

	template <uint8_t S>
	static idx_t GetSignalOffsetWithinRecord(const idx_t signal_idx, const vector<EDFSignalHeader> &signal_hdrs) {
		idx_t offset = 0;
		for (idx_t i = 0; i < signal_idx; i++) {
			const EDFSignalHeader &hdr = signal_hdrs[i];
			offset += hdr.samples_per_record;
		}
		return offset * S;
	}

	static idx_t GetNumberOfAnnotationSignals(const vector<EDFSignalHeader> &);
	static idx_t GetNumberOfNonAnnotationSignals(const vector<EDFSignalHeader> &);

	static TimestampedAnnotationList ParseTAL(const string &);
	static vector<TimestampedAnnotationList> ParseRecordTALs(const string &);
};

} // namespace duckdb
