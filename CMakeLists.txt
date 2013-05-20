cmake_minimum_required( VERSION 2.8 )
project( cpptoml )

option( USE_LIBCXX "Use libc++ for the C++ standard library" ON )

include_directories( "include/" )

if( UNIX )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic" )
    if( USE_LIBCXX )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
    endif()
endif()

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
