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

if (BUILD_FUZZ)
  if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "libfuzz requires clang")
  endif()
  set(FUZZING ON)
endif()

set(FUZZ_OPTIONS
  -fsanitize=address,fuzzer
  -fprofile-instr-generate
  -fcoverage-mapping
  )

##### Slave cyclic #####
target_sources(cl_fuzz_slave_cyclic
  PRIVATE
  cl_fuzz_slave_cyclic.c
  )

set_target_properties (cl_fuzz_slave_cyclic
  PROPERTIES
  C_STANDARD 99
  )

target_include_directories(cl_fuzz_slave_cyclic
  PRIVATE
  ${CLINK_SOURCE_DIR}/src
  ${CLINK_SOURCE_DIR}/src/common
  ${CLINK_SOURCE_DIR}/src/slave
  ${CLINK_BINARY_DIR}/src/common
  ${CLINK_BINARY_DIR}/src/slave
  )

target_link_libraries(cl_fuzz_slave_cyclic
  PRIVATE
  clink
  )

##### Slave SLMP #####
target_sources(cl_fuzz_slave_slmp
  PRIVATE
  cl_fuzz_slave_slmp.c
  )

set_target_properties (cl_fuzz_slave_slmp
  PROPERTIES
  C_STANDARD 99
  )

target_include_directories(cl_fuzz_slave_slmp
  PRIVATE
  ${CLINK_SOURCE_DIR}/src
  ${CLINK_SOURCE_DIR}/src/common
  ${CLINK_SOURCE_DIR}/src/slave
  ${CLINK_BINARY_DIR}/src/common
  ${CLINK_BINARY_DIR}/src/slave
  )

target_link_libraries(cl_fuzz_slave_slmp
  PRIVATE
  clink
  )

##### Master cyclic #####
target_sources(cl_fuzz_master_cyclic
  PRIVATE
  cl_fuzz_master_cyclic.c
  )

set_target_properties (cl_fuzz_master_cyclic
  PROPERTIES
  C_STANDARD 99
  )

target_include_directories(cl_fuzz_master_cyclic
  PRIVATE
  ${CLINK_SOURCE_DIR}/src
  ${CLINK_SOURCE_DIR}/src/common
  ${CLINK_SOURCE_DIR}/src/master
  ${CLINK_BINARY_DIR}/src/common
  ${CLINK_BINARY_DIR}/src/master
  )

target_link_libraries(cl_fuzz_master_cyclic
  PRIVATE
  clink
  )

##### Master SLMP #####
target_sources(cl_fuzz_master_slmp
  PRIVATE
  cl_fuzz_master_slmp.c
  )

set_target_properties (cl_fuzz_master_slmp
  PROPERTIES
  C_STANDARD 99
  )

target_include_directories(cl_fuzz_master_slmp
  PRIVATE
  ${CLINK_SOURCE_DIR}/src
  ${CLINK_SOURCE_DIR}/src/common
  ${CLINK_SOURCE_DIR}/src/master
  ${CLINK_BINARY_DIR}/src/common
  ${CLINK_BINARY_DIR}/src/master
  )

target_link_libraries(cl_fuzz_master_slmp
  PRIVATE
  clink
  )

##### C-link library
target_compile_options(clink
  PRIVATE
  $<$<BOOL:${FUZZING}>:-g -DFUZZ_TEST>
  $<$<BOOL:${FUZZING}>:${FUZZ_OPTIONS}>
  )

target_link_options(clink
  PUBLIC
  $<$<BOOL:${FUZZING}>:${FUZZ_OPTIONS}>
  )
