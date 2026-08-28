include(CTest)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(
  TestBiosignalsCpp ${PROJECT_SOURCE_DIR}/src/biosignals_edf_parser.cpp
                    biosignals_test_utils.cpp test_biosignals_edf_parser.cpp)
target_link_libraries(TestBiosignalsCpp duckdb)

add_executable(TestBiosignalsEDFParser main_test_biosignals_edf_parser.cpp)
target_link_libraries(TestBiosignalsEDFParser TestBiosignalsCpp)
add_test(NAME TestBiosignalsEDFParser COMMAND TestBiosignalsEDFParser
                                              ${TEST_DATA_DIR}/edf_example.edf)
