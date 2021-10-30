cmake_minimum_required(VERSION 3.10)
project(libbacker)

find_package(OpenSSL REQUIRED)

set(target_name libbacker)

set(CMAKE_CXX_STANDARD 17)

SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(sources
    backer.cpp
    backer.h
    base-group-set.cpp
    base-group-set.h
    dir-group-set.cpp
    dir-group-set.h
    file-group-set.cpp
    file-group-set.h
    file-index-database.cpp
    file-index-database.h
    file-tree.cpp
    file-tree.h
)

add_library(${target_name} ${sources})

target_include_directories(${target_name} PRIVATE
    external/
    $<TARGET_PROPERTY:katla-core,INTERFACE_INCLUDE_DIRECTORIES>
)

set_target_properties(${target_name}
    PROPERTIES
      OUTPUT_NAME "backer"
)

file(GENERATE OUTPUT inc.txt CONTENT $<TARGET_PROPERTY:katla-core,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_libraries(${target_name} katla-core katla-sqlite OpenSSL::SSL)