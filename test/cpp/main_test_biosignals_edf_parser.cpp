#include "test_biosignals_edf_parser.hpp"

int main(int argc, char *argv[]) {
	TestBiosignalsEDFParser::TestParseRawGlobalHeader(argv[1]);
	TestBiosignalsEDFParser::TestParseGlobalHeader(argv[1]);

	return 0;
}
