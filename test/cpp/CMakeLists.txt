include(CTest)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(BiosignalsTestingUtils biosignals_test_utils.cpp)

add_executable(TestBioSignalsEDFParser test_biosignals_edf_parser.cpp)
target_link_libraries(TestBioSignalsEDFParser ${TARGET_NAME}_extension duckdb
                      BiosignalsTestingUtils)
add_test(NAME TestBioSignalsEDFParser COMMAND TestBioSignalsEDFParser
                                              ${TEST_DATA_DIR}/edf_example.edf)
