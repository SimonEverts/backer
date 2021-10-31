cmake_minimum_required(VERSION 3.10)
project(backer)

find_package(OpenSSL REQUIRED)

set(target_name backer)

set(CMAKE_CXX_STANDARD 17)

set(sources
    main.cpp
)

add_executable(${target_name} ${sources})

target_include_directories(${target_name} PRIVATE
    ../
    ../../external/cxxopts/include
    $<TARGET_PROPERTY:katla-core,INTERFACE_INCLUDE_DIRECTORIES>
)

file(GENERATE OUTPUT inc.txt CONTENT $<TARGET_PROPERTY:katla-core,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(${target_name} katla-core katla-sqlite libbacker OpenSSL::SSL)