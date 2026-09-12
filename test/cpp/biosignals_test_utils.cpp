#include <cstring>
#include <sstream>
#include <stdexcept>
#include "biosignals_test_utils.hpp"

duckdb::unique_ptr<duckdb::CachingFileHandle> BiosignalsTestUtils::GetCachingFileHandle(duckdb::DuckDB &db,
                                                                                        const char *fp) {
	duckdb::shared_ptr<duckdb::DatabaseInstance> db_instance = db.instance;
	duckdb::FileSystem &fs = db_instance->GetFileSystem();
	duckdb::CachingFileSystem caching_fs(fs, *db_instance);
	std::string open_fp(fp);
	return caching_fs.OpenFile(open_fp, duckdb::FileOpenFlags::FILE_FLAGS_READ);
}

void BiosignalsTestUtils::AssertStringEquals(const char *actual, const char *expected) {
	if (std::strcmp(actual, expected) != 0) {
		std::ostringstream oss;
		oss << "Actual value (" << actual << ") does not match expected value (" << expected << ")";
		throw std::runtime_error(oss.str());
	}
}

void BiosignalsTestUtils::AssertStringEquals(const std::string &actual, const std::string &expected) {
	if (expected.compare(actual) != 0) {
		std::ostringstream oss;
		oss << "Actual value (" << actual << ") does not match expected value (" << expected << ")";
		throw std::runtime_error(oss.str());
	}
}

void BiosignalsTestUtils::CompareRawEDFGlobalHeader(const duckdb::RawEDFGlobalHeader *actual,
                                                    const duckdb::RawEDFGlobalHeader *expected, const bool is_bdf) {
	if (is_bdf) {
		BiosignalsTestUtils::AssertEquals<char>(actual->version[0], expected->version[0]);
		BiosignalsTestUtils::AssertStringEquals(actual->version + 1, expected->version + 1);
	} else {
		BiosignalsTestUtils::AssertStringEquals(actual->version, expected->version);
	}
	BiosignalsTestUtils::AssertStringEquals(actual->patient_id, expected->patient_id);
	BiosignalsTestUtils::AssertStringEquals(actual->recording_id, expected->recording_id);
	BiosignalsTestUtils::AssertStringEquals(actual->start_date, expected->start_date);
	BiosignalsTestUtils::AssertStringEquals(actual->start_time, expected->start_time);
	BiosignalsTestUtils::AssertStringEquals(actual->num_header_bytes, expected->num_header_bytes);
	BiosignalsTestUtils::AssertStringEquals(actual->reserved, expected->reserved);
	BiosignalsTestUtils::AssertStringEquals(actual->num_records, expected->num_records);
	BiosignalsTestUtils::AssertStringEquals(actual->duration_record, expected->duration_record);
	BiosignalsTestUtils::AssertStringEquals(actual->num_signals, expected->num_signals);
}

void BiosignalsTestUtils::CompareEDFGlobalHeader(const duckdb::EDFGlobalHeader *actual,
                                                 const duckdb::EDFGlobalHeader *expected) {
	BiosignalsTestUtils::AssertStringEquals(actual->version, expected->version);
	BiosignalsTestUtils::AssertStringEquals(actual->patient_id, expected->patient_id);
	BiosignalsTestUtils::AssertStringEquals(actual->recording_id, expected->recording_id);

	BiosignalsTestUtils::AssertEquals<unsigned int>(actual->start_date_year, expected->start_date_year);
	BiosignalsTestUtils::AssertEquals<unsigned int>(actual->start_date_month, expected->start_date_month);
	BiosignalsTestUtils::AssertEquals<unsigned int>(actual->start_date_day, expected->start_date_day);

	BiosignalsTestUtils::AssertEquals<unsigned int>(actual->start_time_hour, expected->start_time_hour);
	BiosignalsTestUtils::AssertEquals<unsigned int>(actual->start_time_minute, expected->start_time_minute);
	BiosignalsTestUtils::AssertEquals<unsigned int>(actual->start_time_seconds, expected->start_time_seconds);

	BiosignalsTestUtils::AssertEquals<size_t>(actual->num_header_bytes, expected->num_header_bytes);
	BiosignalsTestUtils::AssertStringEquals(actual->reserved, expected->reserved);
	BiosignalsTestUtils::AssertEquals<int>(actual->num_records, expected->num_records);
	BiosignalsTestUtils::AssertEquals<float>(actual->duration_record, expected->duration_record);
	BiosignalsTestUtils::AssertEquals<int>(actual->num_signals, expected->num_signals);

	BiosignalsTestUtils::AssertEquals<bool>(actual->is_bdf, expected->is_bdf);
	BiosignalsTestUtils::AssertEquals<bool>(actual->is_plus, expected->is_plus);
	BiosignalsTestUtils::AssertEquals<bool>(actual->is_discontinuous, expected->is_discontinuous);
}

void BiosignalsTestUtils::CompareRawEDFSignalHeader(const duckdb::RawEDFSignalHeader *actual,
                                                    const duckdb::RawEDFSignalHeader *expected) {
	BiosignalsTestUtils::AssertStringEquals(actual->label, expected->label);
	BiosignalsTestUtils::AssertStringEquals(actual->transducer_type, expected->transducer_type);
	BiosignalsTestUtils::AssertStringEquals(actual->physical_dim, expected->physical_dim);
	BiosignalsTestUtils::AssertStringEquals(actual->physical_min, expected->physical_min);
	BiosignalsTestUtils::AssertStringEquals(actual->physical_max, expected->physical_max);
	BiosignalsTestUtils::AssertStringEquals(actual->digital_min, expected->digital_min);
	BiosignalsTestUtils::AssertStringEquals(actual->digital_max, expected->digital_max);
	BiosignalsTestUtils::AssertStringEquals(actual->prefiltering, expected->prefiltering);
	BiosignalsTestUtils::AssertStringEquals(actual->samples_per_record, expected->samples_per_record);
	BiosignalsTestUtils::AssertStringEquals(actual->reserved, expected->reserved);
}

void BiosignalsTestUtils::CompareEDFSignalHeader(const duckdb::EDFSignalHeader *actual,
                                                 const duckdb::EDFSignalHeader *expected) {
	BiosignalsTestUtils::AssertStringEquals(actual->label, expected->label);
	BiosignalsTestUtils::AssertStringEquals(actual->transducer_type, expected->transducer_type);
	BiosignalsTestUtils::AssertStringEquals(actual->physical_dim, expected->physical_dim);
	BiosignalsTestUtils::AssertEquals<float>(actual->physical_min, expected->physical_min);
	BiosignalsTestUtils::AssertEquals<float>(actual->physical_max, expected->physical_max);
	BiosignalsTestUtils::AssertEquals<int>(actual->digital_min, expected->digital_min);
	BiosignalsTestUtils::AssertEquals<int>(actual->digital_max, expected->digital_max);
	BiosignalsTestUtils::AssertStringEquals(actual->prefiltering, expected->prefiltering);
	BiosignalsTestUtils::AssertEquals<unsigned int>(actual->samples_per_record, expected->samples_per_record);
	BiosignalsTestUtils::AssertStringEquals(actual->reserved, expected->reserved);
	BiosignalsTestUtils::AssertEquals<bool>(actual->is_annotation, expected->is_annotation);
}
