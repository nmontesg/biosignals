cmake_minimum_required(VERSION 3.5)

set(TARGET_NAME biosignals)

set(EXTENSION_NAME ${TARGET_NAME}_extension)
set(LOADABLE_EXTENSION_NAME ${TARGET_NAME}_loadable_extension)

project(${TARGET_NAME})

set(CMAKE_CXX_STANDARD
    "17"
    CACHE STRING "C++ standard to enforce")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(
  Boost
  COMPONENTS algorithm endian lexical_cast
  REQUIRED)
if(Boost_FOUND)
  message(STATUS "Found Boost: ${Boost_INCLUDE_DIRS}")
  include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(CURL)
link_libraries(CURL::libcurl)

include_directories(src/include)

if($ENV{DEVELOPMENT_VERSION})
  message(STATUS "Compiling development version")
  add_compile_definitions(INSPECT_READ_PERFORMANCE)
  add_compile_definitions(EXTENSION_DEV_VERSION)
endif()

set(EXTENSION_SOURCES
    src/biosignals_edf_parser.cpp
    src/biosignals_extension.cpp
    src/read_edf_global_header.cpp
    src/read_edf_signal_headers.cpp
    src/read_edf_signal.cpp
    src/read_edf_annotations.cpp
    src/medical_analysis_tools_utils.cpp)
add_compile_definitions(PRIVATE BIOSIGNALS_WIN32_EXPORT)

build_static_extension(${TARGET_NAME} ${EXTENSION_SOURCES})
build_loadable_extension(${TARGET_NAME} " " ${EXTENSION_SOURCES})

if($ENV{LOCAL_BUILD})
  message(STATUS "Local build will compile biosignals C++ tests")
  add_subdirectory(test/cpp)
endif()

install(
  TARGETS ${EXTENSION_NAME}
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
