#include "biosignals_edf_parser.hpp"

namespace duckdb {

pair<const char *, size_t> BiosignalsEDFParser::RemoveTrailingSpaces(const char *data, size_t size) {
	if (!data || size == 0) {
		return {data, 0};
	}

	size_t end = size;
	while (end > 0 && data[end - 1] == ' ') {
		--end;
	}

	return {data, end};
}

RawEDFGlobalHeader BiosignalsEDFParser::ParseRawGlobalHeader(FileHandle &fh) {
	RawEDFGlobalHeader raw_header;
	fh.Seek(0);
	fh.Read(reinterpret_cast<char *>(&raw_header), sizeof(RawEDFGlobalHeader));
	return raw_header;
}

} // namespace duckdb
