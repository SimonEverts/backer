cmake_minimum_required(VERSION 3.10)
project(backer)

include(cmake/cmake-common.txt)

option(USE_SYSTEM_GSL "Use system GSL library" ON)
option(USE_SYSTEM_LIBWEBSOCKETS "use system libwebsockets" OFF) # using included libwebsockets as system version on ubuntu 1804 is too old
option(USE_SYSTEM_LIBUV "use system libuv" OFF)

IF(USE_SYSTEM_GSL)
    find_package(GSL REQUIRED)
    add_library(Microsoft.GSL::GSL ALIAS GSL::gsl)
ELSE()
    add_subdirectory(external/gsl)
    include_directories(external/gsl/include)
ENDIF()

find_package(OpenSSL REQUIRED)

option(BUILD_GMOCK "Builds the googlemock subproject" OFF)
option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)
add_subdirectory(external/googletest)

find_package(Threads REQUIRED)
add_subdirectory(external/katla/core)

find_package(SQLite3 REQUIRED)
add_subdirectory(external/katla/sqlite)

add_subdirectory(src/libbacker)
add_subdirectory(src/backer)

add_subdirectory(external/katla/window-management)
add_subdirectory(external/katla/ui-components)

find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)
add_subdirectory(external/katla/gtk)

add_subdirectory(src/backer-gtk)

#add_subdirectory(tests/unit-tests)
