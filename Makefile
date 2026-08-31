PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

EXT_NAME=biosignals
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

include extension-ci-tools/makefiles/duckdb_extension.Makefile

test_cpp_debug: debug
	ctest --test-dir build/debug/extension/biosignals/test/cpp --verbose

test_cpp_release: release
	ctest --test-dir build/debug/extension/biosignals/test/cpp
