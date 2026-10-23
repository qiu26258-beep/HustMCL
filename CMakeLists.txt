cmake_minimum_required(VERSION 3.24)
project(HustMCL VERSION 0.1.0 LANGUAGES CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "HustMCL requires Windows x64.")
endif()
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#if !defined(_M_X64) && !defined(__x86_64__)
#error HustMCL requires x64, not ARM64 or x86
#endif
int main() { return 0; }
" HUSTMCL_TARGET_X64)
if(NOT HUSTMCL_TARGET_X64)
    message(FATAL_ERROR "HustMCL requires an x64 compiler target.")
endif()

option(HUSTMCL_BUILD_DESKTOP "Build the optional Qt Widgets desktop demo" OFF)
option(HUSTMCL_BUILD_METADATA "Build the optional Qt Core version metadata adapter" OFF)
option(HUSTMCL_BUILD_BACKEND "Build Qt Core launch preparation and runtime probes" OFF)

include(CTest)
include(cmake/Architecture.cmake)
add_subdirectory(src/core)
hustmcl_check_core_dependencies(hustmcl_core)

if(BUILD_TESTING)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    add_subdirectory(tests)
endif()

if(HUSTMCL_BUILD_DESKTOP OR HUSTMCL_BUILD_METADATA OR HUSTMCL_BUILD_BACKEND)
    find_package(Qt6 6.10 REQUIRED COMPONENTS Core)
    if(HUSTMCL_BUILD_DESKTOP OR HUSTMCL_BUILD_BACKEND)
        add_subdirectory(src/application)
    endif()
    add_subdirectory(src/services)
endif()

if(HUSTMCL_BUILD_DESKTOP)
    find_package(Qt6 6.10 REQUIRED COMPONENTS Widgets)
    add_subdirectory(src/ui)
endif()

if(HUSTMCL_BUILD_DESKTOP OR HUSTMCL_BUILD_BACKEND)
    add_subdirectory(src/app)
endif()

if(HUSTMCL_BUILD_DESKTOP)
    if(BUILD_TESTING)
        find_package(Qt6 6.10 REQUIRED COMPONENTS Test)
        add_subdirectory(tests/desktop)
    endif()
endif()
