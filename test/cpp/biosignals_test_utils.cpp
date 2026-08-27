#include <cstring>
#include <sstream>
#include <stdexcept>
#include "biosignals_test_utils.hpp"

void BiosignalsTestUtils::AssertStringEquals(const char *actual, const char *expected) {
	if (std::strcmp(actual, expected) != 0) {
		std::ostringstream oss;
		oss << "Actual value (" << actual << ") does not match expected value (" << expected << ")";
		throw std::runtime_error(oss.str());
	}
}
