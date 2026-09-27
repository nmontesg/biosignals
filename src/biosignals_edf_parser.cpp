#include "biosignals_edf_parser.hpp"
#include "boost/algorithm/string/split.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/vector.hpp"
#include "duckdb/storage/external_file_cache/caching_file_system.hpp"

namespace duckdb {

void BiosignalsEDFParser::CopyFromBuffer(char *buf, idx_t &offset, size_t num_bytes, char *dest) {
	memcpy(dest, buf + offset, num_bytes);
	dest[num_bytes] = '\0';
	offset += num_bytes;
}

RawEDFGlobalHeader BiosignalsEDFParser::ParseRawGlobalHeader(unique_ptr<CachingFileHandle> &fh) {
	RawEDFGlobalHeader raw_header;
	char buffer[GLOBAL_HEADER_SIZE];
	idx_t offset = 0;
	fh->Seek(offset);
	idx_t nr_bytes = GLOBAL_HEADER_SIZE;
	FileBufferHandleGroup fh_group = fh->Read(nr_bytes);
	fh_group.CopyTo(data_ptr_cast(buffer), nr_bytes);

	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.version) - 1, raw_header.version);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.patient_id) - 1, raw_header.patient_id);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.recording_id) - 1, raw_header.recording_id);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.start_date) - 1, raw_header.start_date);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.start_time) - 1, raw_header.start_time);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.num_header_bytes) - 1,
	                                    raw_header.num_header_bytes);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.reserved) - 1, raw_header.reserved);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.num_records) - 1, raw_header.num_records);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.duration_record) - 1,
	                                    raw_header.duration_record);
	BiosignalsEDFParser::CopyFromBuffer(buffer, offset, sizeof(raw_header.num_signals) - 1, raw_header.num_signals);

	return raw_header;
}

vector<unsigned int> BiosignalsEDFParser::ValidateDateTime(const string &dt, bool is_date) {
	if (dt.size() != 8 || dt[2] != '.' || dt[5] != '.') {
		throw IOException("Invalid start date or time field in EDF header: " + dt);
	}

	vector<unsigned int> result;

	unsigned int first = boost::lexical_cast<unsigned int>(dt.substr(0, 2));
	result.push_back(first);

	unsigned int second = boost::lexical_cast<unsigned int>(dt.substr(3, 2));
	result.push_back(second);

	unsigned int third = boost::lexical_cast<unsigned int>(dt.substr(6, 2));
	if (is_date) {
		unsigned int century = 2000;
		if (third >= 85) {
			century = 1900;
		}
		third += century;
	}
	result.push_back(third);

	return result;
}

bool BiosignalsEDFParser::CheckBDFVersion(const char *hdr_version) {
	const uint8_t expected_bdf_version[8] = {0xFF, 0x42, 0x49, 0x4F, 0x53, 0x45, 0x4D, 0x49};
	return std::memcmp(hdr_version, expected_bdf_version, 8) == 0;
}

EDFGlobalHeader BiosignalsEDFParser::ParseGlobalHeader(unique_ptr<CachingFileHandle> &fh) {
	RawEDFGlobalHeader raw_header = ParseRawGlobalHeader(fh);
	EDFGlobalHeader header;

	if (BiosignalsEDFParser::CheckBDFVersion(raw_header.version)) {
		BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_header.version + 1, header.version);
		header.is_bdf = true;
	} else {
		BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_header.version, header.version);
	}
	BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_header.patient_id, header.patient_id);
	BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_header.recording_id, header.recording_id);

	string start_date;
	BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_header.start_date, start_date);
	vector<unsigned int> start_date_parsed = BiosignalsEDFParser::ValidateDateTime(start_date, true);
	header.start_date_day = start_date_parsed[0];
	header.start_date_month = start_date_parsed[1];
	header.start_date_year = start_date_parsed[2];

	string start_time;
	BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_header.start_time, start_time);
	vector<unsigned int> start_time_parsed = BiosignalsEDFParser::ValidateDateTime(start_time, false);
	header.start_time_hour = start_time_parsed[0];
	header.start_time_minute = start_time_parsed[1];
	header.start_time_seconds = start_time_parsed[2];

	BiosignalsEDFParser::ParseFieldFromBuffer<size_t>(raw_header.num_header_bytes, header.num_header_bytes);
	BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_header.reserved, header.reserved);
	BiosignalsEDFParser::ParseFieldFromBuffer<int>(raw_header.num_records, header.num_records);
	BiosignalsEDFParser::ParseFieldFromBuffer<float>(raw_header.duration_record, header.duration_record);
	BiosignalsEDFParser::ParseFieldFromBuffer<int>(raw_header.num_signals, header.num_signals);

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

vector<RawEDFSignalHeader> BiosignalsEDFParser::ParseRawSignalHeaders(unique_ptr<CachingFileHandle> &fh,
                                                                      const EDFGlobalHeader &global_hdr) {
	int num_signals = global_hdr.num_signals;
	if (num_signals <= 0) {
		throw IOException("Cannot parse EDF header with non-positive number of signals (" +
		                  std::to_string(num_signals) + ")");
	}

	vector<RawEDFSignalHeader> signal_headers(num_signals);

	idx_t nr_bytes = (size_t)num_signals * SIGNAL_HEADER_SIZE;
	vector<char> buffer(nr_bytes);

	idx_t file_offset = GLOBAL_HEADER_SIZE;
	fh->Seek(file_offset);

	FileBufferHandleGroup fh_group = fh->Read(nr_bytes, file_offset);
	fh_group.CopyTo(data_ptr_cast(buffer.data()), nr_bytes);

	idx_t buf_offset = 0;

	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, label);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, transducer_type);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, physical_dim);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, physical_min);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, physical_max);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, digital_min);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, digital_max);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, prefiltering);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, samples_per_record);
	PARSE_EDF_FIELD(buffer.data(), buf_offset, num_signals, signal_headers, reserved);

	return signal_headers;
}

vector<RawEDFSignalHeader> BiosignalsEDFParser::ParseRawSignalHeaders(unique_ptr<CachingFileHandle> &fh) {
	EDFGlobalHeader global_hdr = BiosignalsEDFParser::ParseGlobalHeader(fh);
	return BiosignalsEDFParser::ParseRawSignalHeaders(fh, global_hdr);
}

vector<EDFSignalHeader> BiosignalsEDFParser::ParseSignalHeaders(unique_ptr<CachingFileHandle> &fh,
                                                                const EDFGlobalHeader &global_hdr) {
	vector<RawEDFSignalHeader> raw_signal_hdrs = BiosignalsEDFParser::ParseRawSignalHeaders(fh, global_hdr);
	size_t num_hdrs = raw_signal_hdrs.size();
	vector<EDFSignalHeader> signal_hdrs(num_hdrs);

	for (idx_t i = 0; i < num_hdrs; i++) {
		BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_signal_hdrs[i].label, signal_hdrs[i].label);
		BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_signal_hdrs[i].transducer_type,
		                                                  signal_hdrs[i].transducer_type);

		BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_signal_hdrs[i].physical_dim, signal_hdrs[i].physical_dim);
		BiosignalsEDFParser::ParseFieldFromBuffer<float>(raw_signal_hdrs[i].physical_min, signal_hdrs[i].physical_min);
		BiosignalsEDFParser::ParseFieldFromBuffer<float>(raw_signal_hdrs[i].physical_max, signal_hdrs[i].physical_max);

		BiosignalsEDFParser::ParseFieldFromBuffer<int>(raw_signal_hdrs[i].digital_min, signal_hdrs[i].digital_min);
		BiosignalsEDFParser::ParseFieldFromBuffer<int>(raw_signal_hdrs[i].digital_max, signal_hdrs[i].digital_max);

		BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_signal_hdrs[i].prefiltering, signal_hdrs[i].prefiltering);
		BiosignalsEDFParser::ParseFieldFromBuffer<unsigned int>(raw_signal_hdrs[i].samples_per_record,
		                                                        signal_hdrs[i].samples_per_record);
		BiosignalsEDFParser::ParseFieldFromBuffer<string>(raw_signal_hdrs[i].reserved, signal_hdrs[i].reserved);

		if (global_hdr.is_bdf) {
			signal_hdrs[i].is_annotation = (signal_hdrs[i].label == BDF_ANNOTATIONS) ? true : false;
		} else {
			signal_hdrs[i].is_annotation = (signal_hdrs[i].label == EDF_ANNOTATIONS) ? true : false;
		}
		signal_hdrs[i].ComputeSlopeAndOffset();
	}

	return signal_hdrs;
}

vector<EDFSignalHeader> BiosignalsEDFParser::ParseSignalHeaders(unique_ptr<CachingFileHandle> &fh) {
	EDFGlobalHeader global_hdr = BiosignalsEDFParser::ParseGlobalHeader(fh);
	return BiosignalsEDFParser::ParseSignalHeaders(fh, global_hdr);
}

size_t BiosignalsEDFParser::GetTotalHeaderSize(const size_t n_signals) {
	return GLOBAL_HEADER_SIZE + n_signals * SIGNAL_HEADER_SIZE;
}

idx_t BiosignalsEDFParser::GetNumberOfAnnotationSignals(const vector<EDFSignalHeader> &signal_hdrs) {
	return std::accumulate(signal_hdrs.begin(), signal_hdrs.end(), 0,
	                       [](size_t sum, const EDFSignalHeader &curr) { return sum + (curr.is_annotation ? 1 : 0); });
}

idx_t BiosignalsEDFParser::GetNumberOfNonAnnotationSignals(const vector<EDFSignalHeader> &signal_hdrs) {
	return std::accumulate(signal_hdrs.begin(), signal_hdrs.end(), 0,
	                       [](size_t sum, const EDFSignalHeader &curr) { return sum + (curr.is_annotation ? 0 : 1); });
}

TimestampedAnnotationList BiosignalsEDFParser::ParseTAL(const string &_raw) {
	TimestampedAnnotationList tal;

	std::vector<std::string> components;

	tal.has_duration = (std::find(_raw.begin(), _raw.end(), TAL_DURATION_DELIMITER) != _raw.end());
	boost::split(components, _raw, [](char c) { return c == TAL_DURATION_DELIMITER or c == TAL_ANNOTATION_DELIMITER; });

	tal.onset = boost::lexical_cast<float>(components[0]);

	idx_t _start = (tal.has_duration ? 2 : 1);
	if (tal.has_duration) {
		tal.duration = boost::lexical_cast<float>(components[1]);
	}

	for (idx_t i = _start; i < components.size(); i++) {
		if (!components[i].empty()) {
			tal.annotations.push_back(components[i]);
		}
	}

	return tal;
}

vector<TimestampedAnnotationList> BiosignalsEDFParser::ParseRecordTALs(const string &_raw) {
	std::vector<std::string> raw_tals;
	boost::split(raw_tals, _raw, [](char c) { return c == TAL_DELIMITER; });

	vector<TimestampedAnnotationList> tals;
	for (auto &_tal : raw_tals) {
		if (_tal.empty()) {
			continue;
		}
		TimestampedAnnotationList tal = ParseTAL(_tal);
		tals.push_back(tal);
	}

	return tals;
}

} // namespace duckdb
