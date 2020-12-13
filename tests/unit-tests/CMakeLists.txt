cmake_minimum_required(VERSION 3.10)

project(backer-unit-tests)

include(${CMAKE_SOURCE_DIR}/cmake/cmake-common.txt)

enable_testing()
include(GoogleTest)

enable_testing()

add_definitions(-DCMAKE_SOURCE_DIR=\"${CMAKE_SOURCE_DIR}\")

set(sources
    backer-tests.cpp
)

set(target_name "backer-unit-tests")
add_executable(${target_name} ${sources})

target_include_directories(${target_name} PRIVATE
    ../../src
    ../../
    ../../external/
    ../../external/abseil-cpp/
    ../../external/outcome/
    ${GTEST_INCLUDE_DIRS}
    )

target_link_libraries(${target_name} gtest gtest_main katla-core fmt libbacker)

gtest_discover_tests(${target_name})
