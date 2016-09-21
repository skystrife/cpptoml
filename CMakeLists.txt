cmake_minimum_required(VERSION 3.1.0)
project(cpptoml)

set(cpptoml_VERSION 0.4.0)
set(cpptoml_TOML_VERSION 0.4.0)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CMakePushCheckState)

cmake_push_check_state()

option(ENABLE_LIBCXX "Use libc++ for the C++ standard library" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/meta-cmake)

if(UNIX OR MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if(CMAKE_GENERATOR STREQUAL "Ninja")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
    endif()

    if(ENABLE_LIBCXX)
      find_package(LIBCXX REQUIRED)
      set_libcxx_required_flags()
    endif()
  endif()
endif()

set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_CXX11_STANDARD_COMPILE_OPTION}")

check_cxx_source_compiles("
#include <iomanip>
int main() {
    std::tm t;
    std::put_time(&t, \"%Y\");
    return 0;
}" CPPTOML_HAS_STD_PUT_TIME)

check_cxx_source_runs("
#include <cstring>
#include <ctime>
#include <regex>
int main() {
    std::string to_match = \"2014-02-05T14:30:05Z\";
    std::regex pattern{\"(\\\\\\\\d{4})-(\\\\\\\\d{2})-(\\\\\\\\d{2})T(\\\\\\\\d{2}):(\\\\\\\\d{2}):(\\\\\\\\d{2})Z\"};
    std::match_results<std::string::const_iterator> results;
    std::regex_match(to_match, results, pattern);

    std::tm date;
    std::memset(&date, '\\\\0', sizeof(date));
    date.tm_year = stoi(results[1]) - 1900;
    date.tm_mon = stoi(results[2]) - 1;
    date.tm_mday = stoi(results[3]);
    date.tm_hour = stoi(results[4]);
    date.tm_min = stoi(results[5]);
    date.tm_sec = stoi(results[6]);
    return 0;
}" CPPTOML_HAS_STD_REGEX)

cmake_pop_check_state()

add_library(cpptoml INTERFACE)

target_include_directories(cpptoml INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if (LIBDL_LIBRARY)
  target_link_libraries(cpptoml INTERFACE ${LIBDL_LIBRARY})
endif()

if (CXXABI_LIBRARY)
  target_link_libraries(cpptoml INTERFACE ${CXXABI_LIBRARY})
endif()

if(CPPTOML_HAS_STD_PUT_TIME)
  target_compile_definitions(cpptoml INTERFACE -DCPPTOML_HAS_STD_PUT_TIME=1)
endif()

if (CPPTOML_HAS_STD_REGEX)
  target_compile_definitions(cpptoml INTERFACE -DCPPTOML_HAS_STD_REGEX=1)
endif()

add_executable(parse parse.cpp)
target_link_libraries(parse cpptoml)
set_target_properties(parse PROPERTIES
  CXX_STANDARD 11
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED YES)

add_executable(cpptoml-parser parse_stdin.cpp)
target_link_libraries(cpptoml-parser cpptoml)
set_target_properties(cpptoml-parser PROPERTIES
  CXX_STANDARD 11
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED YES)

add_executable(cpptoml-build build_toml.cpp)
target_link_libraries(cpptoml-build cpptoml)
set_target_properties(cpptoml-build PROPERTIES
  CXX_STANDARD 11
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED YES)

find_package(Doxygen)
if(DOXYGEN_FOUND AND NOT TARGET doc)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cpptoml.doxygen.in
    ${CMAKE_CURRENT_BINARY_DIR}/cpptoml.doxygen @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE}
    ${CMAKE_CURRENT_BINARY_DIR}/cpptoml.doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/cpptoml/cpptomlConfigVersion.cmake
  VERSION ${cpptoml_VERSION}
  COMPATIBILITY SameMajorVersion)
configure_file(cmake/cpptomlConfig.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/cpptoml/cpptomlConfig.cmake
               COPYONLY)

install(TARGETS cpptoml
        EXPORT cpptoml-exports)
install(FILES include/cpptoml.h
        DESTINATION include)
install(EXPORT cpptoml-exports
        FILE cpptomlTargets.cmake
        DESTINATION lib/cmake/cpptoml)
install(FILES
          ${CMAKE_CURRENT_BINARY_DIR}/cpptoml/cpptomlConfigVersion.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/cpptoml/cpptomlConfig.cmake
        DESTINATION
          lib/cmake/cpptoml)

export(EXPORT cpptoml-exports
       FILE ${CMAKE_CURRENT_BINARY_DIR}/cpptoml/cpptomlTargets.cmake)
export(PACKAGE cpptoml)
