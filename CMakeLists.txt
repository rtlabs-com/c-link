#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# http://www.rt-labs.com
# Copyright 2022 rt-labs AB, Sweden. All rights reserved.
#
# See the file LICENSE.md distributed with this software for full
# license information.
#*******************************************************************/

cmake_minimum_required (VERSION 3.14)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools")
project (CLINK VERSION 1.12.0)

# Default settings if this is the main project
if (CMAKE_PROJECT_NAME STREQUAL CLINK)

  # Create a compile_commands.json file for use with clang-tidy
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  include(CTest)

  # Make option visible in ccmake, cmake-gui
  option(BUILD_SHARED_LIBS "Build shared library" OFF)
  option(BUILD_FUZZ "Build fuzz test" OFF)

  # Default to release build with debug info
  if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Coverage"
      FORCE)
  endif (NOT CMAKE_BUILD_TYPE)

  # Default to installing in build directory
  if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CLINK_BINARY_DIR}/install
      CACHE PATH "Default install path" FORCE)
  endif()

  message(STATUS "Current build type is: ${CMAKE_BUILD_TYPE}")
  message(STATUS "Current install path is: ${CMAKE_INSTALL_PREFIX}")
  message(STATUS "Building for ${CMAKE_SYSTEM_NAME}")
endif()

include(AddOsal)
include(GenerateExportHeader)
include(CMakeDependentOption)
include(GetGitRevision)

# Always use standard .o suffix
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

set(LOG_STATE_VALUES "ON;OFF")
set(LOG_LEVEL_VALUES "DEBUG;INFO;WARNING;ERROR;FATAL")

set(LOG_LEVEL FATAL CACHE STRING "default log level")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS ${LOG_LEVEL_VALUES})

set(CL_CLAL_LOG ON CACHE STRING "CLAL log")
set_property(CACHE CL_CLAL_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(CL_ETH_LOG ON CACHE STRING "ETH log")
set_property(CACHE CL_ETH_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(CL_CCIEFB_LOG ON CACHE STRING "CCIEFB log")
set_property(CACHE CL_CCIEFB_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(CL_SLMP_LOG ON CACHE STRING "SLMP log")
set_property(CACHE CL_SLMP_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(CL_LOG ON CACHE STRING "c-link log")
set_property(CACHE CL_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(CL_MAX_DIRECTORYPATH_SIZE  "240"
  CACHE STRING "Max directory path size, including termination")

set(CLS_MAX_OCCUPIED_STATIONS "4"
  CACHE STRING "Max number of occupied slave stations. Allowed 1..16 Tests use 4.")

set(CLM_MAX_GROUPS "5"
  CACHE STRING "Max number of groups. Allowed 1..64 Tests use 5.")

set(CLM_MAX_OCCUPIED_STATIONS_PER_GROUP "13"
  CACHE STRING "Max number of occupied slave stations per group. Allowed 1..16 Tests use 13.")

set(CLM_MAX_NODE_SEARCH_DEVICES "20"
  CACHE STRING "Max number of node search response stored in database")

# Generate version numbers
configure_file (
  include/cl_version.h.in
  ${CLINK_BINARY_DIR}/include/cl_version.h
  )

# Generate config options
configure_file (
  include/cl_options.h.in
  ${CLINK_BINARY_DIR}/include/cl_options.h
  )

# Add platform-dependent targets early, so they can be configured by
# platform
add_library(clink "")
if (CMAKE_PROJECT_NAME STREQUAL CLINK AND NOT BUILD_FUZZ)
  add_executable(cl_sample_slave "")
  add_executable(cl_sample_master "")
endif()


if (CMAKE_PROJECT_NAME STREQUAL CLINK AND BUILD_TESTING AND NOT BUILD_FUZZ)
  add_executable(cl_test "")
endif()

if (CMAKE_PROJECT_NAME STREQUAL CLINK AND BUILD_FUZZ)
  add_executable(cl_fuzz_slave_cyclic "")
  add_executable(cl_fuzz_slave_slmp "")
  add_executable(cl_fuzz_master_cyclic "")
  add_executable(cl_fuzz_master_slmp "")
endif()

# Platform configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CMAKE_SYSTEM_NAME}.cmake)

generate_export_header(clink
  BASE_NAME cl
  EXPORT_FILE_NAME ${CLINK_BINARY_DIR}/include/cl_export.h
  )

set_target_properties (clink
  PROPERTIES
  C_STANDARD 99
  )

target_compile_features(clink PUBLIC c_std_99)

target_include_directories(clink
  PUBLIC
  $<BUILD_INTERFACE:${CLINK_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CLINK_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  ${CLINK_BINARY_DIR}/src
  src
  )

target_link_libraries(clink PUBLIC osal)

install (
  TARGETS clink
  EXPORT ClinkConfig
  )

install(
  EXPORT ClinkConfig
  DESTINATION cmake
  )

install (FILES
  include/cl_common.h
  include/cls_api.h
  include/clm_api.h
  ${CLINK_BINARY_DIR}/include/cl_export.h
  ${CLINK_BINARY_DIR}/include/cl_options.h
  ${CLINK_BINARY_DIR}/include/cl_version.h
  DESTINATION include
  )

add_subdirectory(src)
if (CMAKE_PROJECT_NAME STREQUAL CLINK AND NOT BUILD_FUZZ)
  add_subdirectory(sample_apps)
endif()

if (CMAKE_PROJECT_NAME STREQUAL CLINK AND BUILD_TESTING AND NOT BUILD_FUZZ)
  add_subdirectory(test)
  include(AddGoogleTest)
  add_gtest(cl_test)

  # Create a HTML coverage report in <build>/coverage.html
  find_program(GCOVR gcovr)
  if (GCOVR AND CMAKE_BUILD_TYPE MATCHES "Coverage")
    add_custom_target(coverage-report
      COMMAND mkdir -p coverage_html
      COMMAND ${GCOVR}
      --html-details
      --object-directory ${CLINK_BINARY_DIR}
      --html-title "c-link test coverage report"
      --html-medium-threshold 95
      --html-high-threshold 100
      --html-self-contained
      --root ${CLINK_SOURCE_DIR}/src
      --exclude ${CLINK_SOURCE_DIR}/src/ports
      -o coverage_html/coverage.html
      COMMENT "Generating coverage HTML report"
      )
    add_dependencies(coverage-report check)
  endif()
endif()

if (CMAKE_PROJECT_NAME STREQUAL CLINK AND BUILD_FUZZ)
  add_subdirectory(fuzz)
endif()

if (CMAKE_PROJECT_NAME STREQUAL CLINK AND NOT BUILD_FUZZ)
  add_subdirectory(docs)

  add_custom_target(codespell
    COMMAND codespell
        ${CLINK_SOURCE_DIR}/include/
        ${CLINK_SOURCE_DIR}/docs/
        ${CLINK_SOURCE_DIR}/src/
        ${CLINK_SOURCE_DIR}/test/
        --skip *mypy_cache*
    COMMENT "Running spell check on source code"
    )
endif()

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${CLINK_SOURCE_DIR}/LICENSE.md")
set (CPACK_PACKAGE_CONTACT info.c-link@rt-labs.com)
include (CPack)
