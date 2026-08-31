#include "biosignals_edf_parser.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/lexical_cast.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/file_system.hpp"
#include "duckdb/common/vector.hpp"
#include <cstring>

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

	if (header.reserved == "EDF+C") {
		header.is_plus = true;
	} else if (header.reserved == "EDF+D") {
		header.is_plus = true;
		header.is_discontinuous = true;
	} else if (header.reserved == "BDF+C") {
		header.is_plus = true;
	} else if (header.reserved == "BDF+D") {
		header.is_plus = true;
		header.is_discontinuous = true;
	}

	return header;
}

} // namespace duckdb
