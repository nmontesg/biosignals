#include "biosignals_edf_parser.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/lexical_cast.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/file_system.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {

void BiosignalsEDFParser::ParseFieldFromFile(FileHandle &fh, idx_t &offset, size_t num_bytes, char *buffer) {
	fh.Seek(offset);
	fh.Read(buffer, num_bytes);
	buffer[num_bytes] = '\0';
	offset += num_bytes;
}

RawEDFGlobalHeader BiosignalsEDFParser::ParseRawGlobalHeader(FileHandle &fh) {
	RawEDFGlobalHeader raw_header;
	idx_t offset = 0;

	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.version) - 1, raw_header.version);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.patient_id) - 1, raw_header.patient_id);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.recording_id) - 1, raw_header.recording_id);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.start_date) - 1, raw_header.start_date);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.start_time) - 1, raw_header.start_time);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.num_header_bytes) - 1,
	                                        raw_header.num_header_bytes);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.reserved) - 1, raw_header.reserved);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.num_records) - 1, raw_header.num_records);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.duration_record) - 1,
	                                        raw_header.duration_record);
	BiosignalsEDFParser::ParseFieldFromFile(fh, offset, sizeof(raw_header.num_signals) - 1, raw_header.num_signals);

	return raw_header;
}

vector<unsigned int> BiosignalsEDFParser::ValidateDateTime(const string &dt, bool is_date) {
	if (dt.size() != 8 || dt[2] != '.' || dt[5] != '.') {
		throw IOException("Invalid start date or time field in EDF header: " + dt);
	}

	vector<unsigned int> result;

	unsigned int first = boost::lexical_cast<unsigned int>(dt.substr(0, 2));
	if (is_date) {
		unsigned int century = 2000;
		if (first >= 85) {
			century = 1900;
		}
		first += century;
	}
	result.push_back(first);

	unsigned int second = boost::lexical_cast<unsigned int>(dt.substr(3, 2));
	result.push_back(second);

	unsigned int third = boost::lexical_cast<unsigned int>(dt.substr(6, 2));
	result.push_back(third);

	return result;
}

template <typename T>
void BiosignalsEDFParser::ParseFieldFromRawHeader(const char *raw_header_field, T &hdr_field) {
	string raw_field_trimmed = boost::algorithm::trim_right_copy_if(string(raw_header_field), boost::is_space());
	try {
		hdr_field = boost::lexical_cast<T>(raw_field_trimmed);
	} catch (const boost::bad_lexical_cast &) {
		throw IOException("Invalid EDF header field: could not parse \"" + raw_field_trimmed + "\"");
	}
}

bool BiosignalsEDFParser::CheckBDFVersion(const char *hdr_version) {
	const uint8_t expected_bdf_version[8] = {0xFF, 0x42, 0x49, 0x4F, 0x53, 0x45, 0x4D, 0x49};
	return std::memcmp(hdr_version, expected_bdf_version, 8) == 0;
}

EDFGlobalHeader BiosignalsEDFParser::ParseGlobalHeader(FileHandle &fh) {
	RawEDFGlobalHeader raw_header = ParseRawGlobalHeader(fh);
	EDFGlobalHeader header;

	if (BiosignalsEDFParser::CheckBDFVersion(raw_header.version)) {
		BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_header.version + 1, header.version);
		header.is_bdf = true;
	} else {
		BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_header.version, header.version);
	}
	BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_header.patient_id, header.patient_id);
	BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_header.recording_id, header.recording_id);

	string start_date;
	BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_header.start_date, start_date);
	vector<unsigned int> start_date_parsed = BiosignalsEDFParser::ValidateDateTime(start_date, true);
	header.start_date_year = start_date_parsed[0];
	header.start_date_month = start_date_parsed[1];
	header.start_date_day = start_date_parsed[2];

	string start_time;
	BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_header.start_time, start_time);
	vector<unsigned int> start_time_parsed = BiosignalsEDFParser::ValidateDateTime(start_time, false);
	header.start_time_hour = start_time_parsed[0];
	header.start_time_minute = start_time_parsed[1];
	header.start_time_seconds = start_time_parsed[2];

	BiosignalsEDFParser::ParseFieldFromRawHeader<size_t>(raw_header.num_header_bytes, header.num_header_bytes);
	BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_header.reserved, header.reserved);
	BiosignalsEDFParser::ParseFieldFromRawHeader<int>(raw_header.num_records, header.num_records);
	BiosignalsEDFParser::ParseFieldFromRawHeader<float>(raw_header.duration_record, header.duration_record);
	BiosignalsEDFParser::ParseFieldFromRawHeader<int>(raw_header.num_signals, header.num_signals);

	if (header.reserved == EDF_PLUS_CONTINUOUS) {
		header.is_plus = true;
	} else if (header.reserved == EDF_PLUS_DISCONTINUOUS) {
		header.is_plus = true;
		header.is_discontinuous = true;
	} else if (header.reserved == BDF_PLUS_CONTINUOUS) {
		header.is_plus = true;
	} else if (header.reserved == BDF_PLUS_DISCONTINUOUS) {
		header.is_plus = true;
		header.is_discontinuous = true;
	}

	return header;
}

vector<RawEDFSignalHeader> BiosignalsEDFParser::ParseRawSignalHeaders(FileHandle &fh,
                                                                      const EDFGlobalHeader &global_hdr) {
	int num_signals = global_hdr.num_signals;
	if (num_signals <= 0) {
		throw IOException("Cannot parse EDF header with non-positive number of signals (" +
		                  std::to_string(num_signals) + ")");
	}

	vector<RawEDFSignalHeader> signal_headers(num_signals);
	idx_t offset = sizeof(RawEDFGlobalHeader) - 10; // 10 is the number of fields in RawEDFGlobalHeader

	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, label);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, transducer_type);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, physical_dim);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, physical_min);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, physical_max);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, digital_min);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, digital_max);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, prefiltering);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, samples_per_record);
	PARSE_EDF_FIELD(fh, offset, num_signals, signal_headers, reserved);

	return signal_headers;
}

vector<RawEDFSignalHeader> BiosignalsEDFParser::ParseRawSignalHeaders(FileHandle &fh) {
	EDFGlobalHeader global_hdr = BiosignalsEDFParser::ParseGlobalHeader(fh);
	return BiosignalsEDFParser::ParseRawSignalHeaders(fh, global_hdr);
}

vector<EDFSignalHeader> BiosignalsEDFParser::ParseSignalHeaders(FileHandle &fh, const EDFGlobalHeader &global_hdr) {
	vector<RawEDFSignalHeader> raw_signal_hdrs = BiosignalsEDFParser::ParseRawSignalHeaders(fh, global_hdr);
	size_t num_hdrs = raw_signal_hdrs.size();
	vector<EDFSignalHeader> signal_hdrs(num_hdrs);

	for (idx_t i = 0; i < num_hdrs; i++) {
		BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_signal_hdrs[i].label, signal_hdrs[i].label);
		BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_signal_hdrs[i].transducer_type,
		                                                     signal_hdrs[i].transducer_type);

		BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_signal_hdrs[i].physical_dim,
		                                                     signal_hdrs[i].physical_dim);
		BiosignalsEDFParser::ParseFieldFromRawHeader<float>(raw_signal_hdrs[i].physical_min,
		                                                    signal_hdrs[i].physical_min);
		BiosignalsEDFParser::ParseFieldFromRawHeader<float>(raw_signal_hdrs[i].physical_max,
		                                                    signal_hdrs[i].physical_max);

		BiosignalsEDFParser::ParseFieldFromRawHeader<int>(raw_signal_hdrs[i].digital_min, signal_hdrs[i].digital_min);
		BiosignalsEDFParser::ParseFieldFromRawHeader<int>(raw_signal_hdrs[i].digital_max, signal_hdrs[i].digital_max);

		BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_signal_hdrs[i].prefiltering,
		                                                     signal_hdrs[i].prefiltering);
		BiosignalsEDFParser::ParseFieldFromRawHeader<unsigned int>(raw_signal_hdrs[i].samples_per_record,
		                                                           signal_hdrs[i].samples_per_record);
		BiosignalsEDFParser::ParseFieldFromRawHeader<string>(raw_signal_hdrs[i].reserved, signal_hdrs[i].reserved);

		if (global_hdr.is_bdf) {
			signal_hdrs[i].is_annotation = (signal_hdrs[i].label == BDF_ANNOTATIONS) ? true : false;
		} else {
			signal_hdrs[i].is_annotation = (signal_hdrs[i].label == EDF_ANNOTATIONS) ? true : false;
		}
		signal_hdrs[i].ComputeSlopeAndOffset();
	}

	return signal_hdrs;
}

vector<EDFSignalHeader> BiosignalsEDFParser::ParseSignalHeaders(FileHandle &fh) {
	EDFGlobalHeader global_hdr = BiosignalsEDFParser::ParseGlobalHeader(fh);
	return BiosignalsEDFParser::ParseSignalHeaders(fh, global_hdr);
}

size_t BiosignalsEDFParser::GetTotalHeaderSize(const size_t n_signals) {
	// there are 10 fields in RawEDFGlobalHeader
	// there are 10 fields in RawEDFSignalHeader
	return sizeof(RawEDFGlobalHeader) - 10 + n_signals * (sizeof(RawEDFSignalHeader) - 10);
}

template <size_t S>
size_t BiosignalsEDFParser::GetRecordSize(const vector<EDFSignalHeader> &signal_hdrs) {
	size_t record_sz = 0;
	for (const EDFSignalHeader &hdr : signal_hdrs) {
		record_sz += (hdr.is_annotation ? hdr.samples_per_record : S * hdr.samples_per_record);
	}
	return record_sz;
}

template <size_t S>
idx_t BiosignalsEDFParser::GetSignalOffsetWithinRecord(const idx_t signal_idx,
                                                       const vector<EDFSignalHeader> &signal_hrs) {
	idx_t offset = 0;
	for (idx_t i; i < signal_idx; i++) {
		const EDFSignalHeader &hdr = signal_hrs[i];
		offset += (hdr.is_annotation ? hdr.samples_per_record : S * hdr.samples_per_record);
	}
	return offset;
}

} // namespace duckdb
