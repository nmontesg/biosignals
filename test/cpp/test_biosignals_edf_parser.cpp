#include "biosignals_edf_parser.hpp"
#include "duckdb/common/local_file_system.hpp"

int main() {
	duckdb::LocalFileSystem fs;
	std::cout << fs.GetWorkingDirectory() << '\n';
	return 0;
}
