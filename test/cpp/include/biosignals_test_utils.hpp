#pragma once

#include <sstream>
#include <string>

class BiosignalsTestUtils {
public:
	static void AssertStringEquals(const char *, const char *);
	static void AssertStringEquals(const std::string &, const std::string &);

	template <typename T>
	static void AssertEquals(const T actual, const T expected) {
		if (actual != expected) {
			std::ostringstream oss;
			oss << "Actual value (" << actual << ") does not match expected value (" << expected << ")";
			throw std::runtime_error(oss.str());
		}
	}
};
