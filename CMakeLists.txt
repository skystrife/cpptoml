cmake_minimum_required( VERSION 2.8 )
project( cpptoml )

option( USE_LIBCXX "Use libc++ for the C++ standard library" ON )

include_directories( "include/" )

if( UNIX )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic" )
    set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_CXX_EXE_LINKER_FLAGS} -ldl" )
    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
        if( USE_LIBCXX )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++abi")
            set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -lc++abi")
        endif()
    endif()
endif()

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
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
    std::memset( &date, '\\\\0', sizeof( date ) );
    date.tm_year = stoi( results[1] ) - 1900;
    date.tm_mon = stoi( results[2] ) - 1;
    date.tm_mday = stoi( results[3] );
    date.tm_hour = stoi( results[4] );
    date.tm_min = stoi( results[5] );
    date.tm_sec = stoi( results[6] );
    return 0;
}" CPPTOML_HAS_STD_REGEX)

add_executable( parse parse.cpp )
add_executable( cpptoml parse_stdin.cpp )

find_package( Doxygen )
if( DOXYGEN_FOUND )
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cpptoml.doxygen.in
                    ${CMAKE_CURRENT_BINARY_DIR}/cpptoml.doxygen @ONLY
    )
    add_custom_target( doc
                       ${DOXYGEN_EXECUTABLE}
                       ${CMAKE_CURRENT_BINARY_DIR}/cpptoml.doxygen
                       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()
