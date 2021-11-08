# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

cmake_minimum_required(VERSION 3.13)

# [option] CMAKE_BUILD_TYPE (default: "Release")
# Build in one of the following modes: Release, Debug, MiniSizeRel, or RelWithDebInfo.
# Most generators recognize these and can set the compiler flags accordingly. We set
# the build type here before creating the project to prevent the CMake generator from
# overriding our default of "Release".
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS "Release" "Debug" "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "Build type (CMAKE_BUILD_TYPE): ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # In Debug mode, enable extra compiler flags.
    include(CheckCXXCompilerFlag)
    # For easier adding of CXX compiler flags
    function(se_enable_cxx_compiler_flag_if_supported flag)
        string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
        if(flag_already_set EQUAL -1)
            message(STATUS "Adding CXX compiler flag: ${flag} ...")
            check_cxx_compiler_flag("${flag}" flag_supported)
            if(flag_supported)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
            endif()
            unset(flag_supported CACHE)
        endif()
    endfunction()
    if(NOT MSVC AND SEAL_DEBUG)
        se_enable_cxx_compiler_flag_if_supported("-Wall")
        se_enable_cxx_compiler_flag_if_supported("-Wextra")
        se_enable_cxx_compiler_flag_if_supported("-Wconversion")
        se_enable_cxx_compiler_flag_if_supported("-Wshadow")
        se_enable_cxx_compiler_flag_if_supported("-pedantic")
    endif()
endif()

project(SEAL_Embedded_Adapter VERSION 1.1.0 LANGUAGES CXX)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
    cmake_policy(SET CMP0077 NEW)
endif()

# [option] SE_ADAPTER_USE_CXX17_OPTION_STR (default: ON)
# Use C++17, use C++14 otherwise. An error will be thrown if SEAL_USE_CXX17 is ON but SE_ADAPTER_USE_CXX17 is OFF.
set(SE_ADAPTER_USE_CXX17_OPTION_STR "Use C++17")
option(SE_ADAPTER_USE_CXX17 ${SE_ADAPTER_USE_CXX17_OPTION_STR} ON)
message(STATUS "SE_ADAPTER_USE_CXX17: ${SE_ADAPTER_USE_CXX17}")

# Enable security-related compile options (MSVC only)
set(SE_ADAPTER_SECURE_COMPILE_OPTIONS_OPTION_STR "Enable Control Flow Guard and Spectre mitigations (MSVC only)")
option(SE_ADAPTER_SECURE_COMPILE_OPTIONS ${SE_ADAPTER_SECURE_COMPILE_OPTIONS_OPTION_STR} OFF)
mark_as_advanced(SE_ADAPTER_SECURE_COMPILE_OPTIONS)

# Source tree
set(SE_ADAPTER_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(SE_ADAPTER_CONFIG_IN_FILENAME ${CMAKE_CURRENT_LIST_DIR}/config.h.in)
set(SE_ADAPTER_THIRDPARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/thirdparty)

# Build tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${OUTLIB_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${OUTLIB_PATH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(SE_ADAPTER_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(SE_ADAPTER_CONFIG_H_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/config.h)

#########################
# External Dependencies #
#########################

# [option] SE_ADAPTER_BUILD_DEPS (default: ON)
# Download and build missing dependencies, throw error if disabled.
set(SE_ADAPTER_BUILD_DEPS_OPTION_STR "Automatically download and build unmet dependencies")
option(SE_ADAPTER_BUILD_DEPS ${SE_ADAPTER_BUILD_DEPS_OPTION_STR} ON)
message(STATUS "SE_ADAPTER_BUILD_DEPS: ${SE_ADAPTER_BUILD_DEPS}")

if(SE_ADAPTER_BUILD_DEPS)
    include(FetchContent)
    mark_as_advanced(FETCHCONTENT_BASE_DIR)
    mark_as_advanced(FETCHCONTENT_FULLY_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED)
    mark_as_advanced(FETCHCONTENT_QUIET)
endif()

# Microsoft SEAL
if(SE_ADAPTER_BUILD_DEPS)
    message(STATUS "Microsoft SEAL: download ...")
    set(SE_ADAPTER_FETCHCONTENT_BASE_DIR_OLD ${FETCHCONTENT_BASE_DIR})
    set(FETCHCONTENT_BASE_DIR ${SE_ADAPTER_THIRDPARTY_DIR} CACHE STRING "" FORCE)
    FetchContent_Declare(
        seal
        GIT_REPOSITORY https://github.com/microsoft/SEAL.git
        GIT_TAG        79234726053c45eede688400aa219fdec0810bd8 # 3.7.2
    )
    FetchContent_GetProperties(seal)

    if(NOT seal_POPULATED)
        FetchContent_Populate(seal)
        set(SEAL_USE_CXX17 ON)
        set(SEAL_BUILD_DEPS ON)
        mark_as_advanced(FETCHCONTENT_SOURCE_DIR_SEAL)
        mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_SEAL)
        message(STATUS ${seal_SOURCE_DIR})
        add_subdirectory(
            ${seal_SOURCE_DIR}
            EXCLUDE_FROM_ALL)
    endif()
    set(FETCHCONTENT_BASE_DIR ${SE_ADAPTER_FETCHCONTENT_BASE_DIR_OLD} CACHE STRING "" FORCE)
    unset(SE_ADAPTER_FETCHCONTENT_BASE_DIR_OLD)
else()
    find_package(SEAL 3.7 QUIET REQUIRED)
    if(NOT SEAL_FOUND)
        message(FATAL_ERROR "Microsoft SEAL: not found")
    else()
        message(STATUS "Microsoft SEAL: found")
    endif()
endif()
if(NOT SE_ADAPTER_USE_CXX17 AND SEAL_USE_CXX17)
    message(FATAL_ERROR "CXX standards mismatch: SE_ADAPTER_USE_CXX17 is OFF, SEAL_USE_CXX17 is ON")
endif()

#################
# Configuration #
#################

# Set a directory to save output files generated by SEAL-Embedded Adapter
set(SE_ADAPTER_FILE_OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/../device CACHE STRING "File output directory" FORCE)
message(STATUS "File output directory: ${SE_ADAPTER_FILE_OUTPUT_DIR}")
if(NOT EXISTS ${SE_ADAPTER_FILE_OUTPUT_DIR}/adapter_output_data)
    make_directory(${SE_ADAPTER_FILE_OUTPUT_DIR}/adapter_output_data)
endif()

# Create the config file
configure_file(${SE_ADAPTER_CONFIG_IN_FILENAME} ${SE_ADAPTER_CONFIG_H_FILENAME})

#####################################
# SEAL-Embedded Adapter C++ library #
#####################################

# TODO: factorize adapter and build a C++ library

####################################
# SEAL-Embedded Adapter Executable #
####################################

add_executable(se_adapter)
target_sources(se_adapter
    PRIVATE
        adapter.cpp
        fileops.cpp
        generate.cpp
        convert.cpp
        utils.cpp
)
target_include_directories(se_adapter PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
if(TARGET SEAL::seal)
    target_link_libraries(se_adapter PRIVATE SEAL::seal)
elseif(TARGET SEAL::seal_shared)
    target_link_libraries(se_adapter PRIVATE SEAL::seal_shared)
else()
    message(FATAL_ERROR "Cannot find target SEAL::seal or SEAL::seal_shared")
endif()
