cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME biosignals)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})

set(CMAKE_CXX_STANDARD "17" CACHE STRING "C++ standard to enforce")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost COMPONENTS algorithm lexical_cast REQUIRED)
if(Boost_FOUND)
    message(STATUS "Found Boost: ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} lfs fetch --all
    COMMAND ${GIT_EXECUTABLE} lfs checkout
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_LFS_OUTPUT
    ERROR_VARIABLE GIT_LFS_ERROR
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
  )
endif()

include_directories(src/include)

set(EXTENSION_SOURCES
    src/biosignals_extension.cpp
    src/biosignals_edf_parser.cpp
    src/read_edf_global_header.cpp)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

set(TEST_DATA_DIR ${CMAKE_SOURCE_DIR}/../test/test_data)
add_subdirectory(test/cpp)

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
