include(CTest)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(
  TestBiosignalsCpp ${PROJECT_SOURCE_DIR}/src/biosignals_edf_parser.cpp
                    biosignals_test_utils.cpp)
target_link_libraries(TestBiosignalsCpp duckdb)

function(add_biosignals_parsing_test TEST_NAME TEST_SCRIPT TEST_DATA_FILE)
  add_executable(${TEST_NAME} ${TEST_SCRIPT})
  target_link_libraries(${TEST_NAME} TestBiosignalsCpp)
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME}
                                     ${TEST_DATA_DIR}/${TEST_DATA_FILE})
endfunction()

# test parsing header for EDF
add_biosignals_parsing_test(
  TestParseEDFRawGlobalHeader test_parse_edf_raw_global_header.cpp
  edf_example.edf)
add_biosignals_parsing_test(TestParseEDFGlobalHeader
                            test_parse_edf_global_header.cpp edf_example.edf)

# test parsing header for EDF+
add_biosignals_parsing_test(
  TestParseEDFPlusRawGlobalHeader test_parse_edf_plus_raw_global_header.cpp
  edf_plus_example.edf)
add_biosignals_parsing_test(
  TestParseEDFPlusGlobalHeader test_parse_edf_plus_global_header.cpp
  edf_plus_example.edf)

# test parsing header for BDF
add_biosignals_parsing_test(
  TestParseBDFRawGlobalHeader test_parse_bdf_raw_global_header.cpp
  bdf_example.bdf)
add_biosignals_parsing_test(TestParseBDFGlobalHeader
                            test_parse_bdf_global_header.cpp bdf_example.bdf)

# test parsing header for BDF+
add_biosignals_parsing_test(
  TestParseBDFPlusRawGlobalHeader test_parse_bdf_plus_raw_global_header.cpp
  bdf_plus_example.bdf)
add_biosignals_parsing_test(
  TestParseBDFPlusGlobalHeader test_parse_bdf_plus_global_header.cpp
  bdf_plus_example.bdf)

# test parsing signal header for EDF
add_biosignals_parsing_test(
  TestParseEDFRawSignalHeaders test_parse_edf_raw_signal_headers.cpp
  edf_example.edf)
add_biosignals_parsing_test(TestParseEDFSignalHeaders
                            test_parse_edf_signal_headers.cpp edf_example.edf)

# test parsing signal header for EDF+
add_biosignals_parsing_test(
  TestParseEDFPlusRawSignalHeaders test_parse_edf_plus_raw_signal_headers.cpp
  edf_plus_example.edf)
add_biosignals_parsing_test(
  TestParseEDFPlusSignalHeaders test_parse_edf_plus_signal_headers.cpp
  edf_plus_example.edf)

# test parsing signal header for BDF
add_biosignals_parsing_test(
  TestParseBDFRawSignalHeaders test_parse_bdf_raw_signal_headers.cpp
  bdf_example.bdf)
add_biosignals_parsing_test(TestParseBDFSignalHeaders
                            test_parse_bdf_signal_headers.cpp bdf_example.bdf)

# test parsing signal header for BDF+
add_biosignals_parsing_test(
  TestParseBDFPlusRawSignalHeaders test_parse_bdf_plus_raw_signal_headers.cpp
  bdf_plus_example.bdf)
add_biosignals_parsing_test(
  TestParseBDFPlusSignalHeaders test_parse_bdf_plus_signal_headers.cpp
  bdf_plus_example.bdf)

# test parsing annotation
add_biosignals_parsing_test(
  TestParseEDFPlusAnnotations test_parse_edf_plus_annotations.cpp
  SC4001EC-Hypnogram.edf)
