cmake_minimum_required(VERSION 3.10)
project(backer)

include(cmake/cmake-common.txt)

option(BUILD_GMOCK "Builds the googlemock subproject" OFF)
option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)
add_subdirectory(external/googletest)

add_subdirectory(external/katla/core)
add_subdirectory(external/katla/sqlite)

add_subdirectory(src/libbacker)
add_subdirectory(src/backer)

add_subdirectory(tests/unit-tests)
