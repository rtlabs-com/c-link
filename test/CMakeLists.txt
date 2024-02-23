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

set_target_properties (cl_test
  PROPERTIES
  C_STANDARD 99
  CXX_STANDARD 20
  )

target_sources(cl_test PRIVATE
  # Unit tests
  test_both_master_slave.cpp
  test_common_eth.cpp
  test_common_file.cpp
  test_common_iefb.cpp
  test_common_limiter.cpp
  test_common_literals.cpp
  test_common_slmp_udp.cpp
  test_common_slmp.cpp
  test_common_timer.cpp
  test_common_util.cpp
  test_master_api.cpp
  test_master_iefb.cpp
  test_master_slmp.cpp
  test_master.cpp
  test_memory_functions.cpp
  test_slave_api.cpp
  test_slave_iefb.cpp
  test_slave_slmp.cpp
  test_slave.cpp

  # Test utils
  mocks.h
  mocks.cpp
  utils_for_testing.h
  utils_for_testing.cpp

  # Testrunner
  cl_test.cpp
  )

# Rebuild units to be tested with UNIT_TEST flag set. This is used to
# mock external dependencies.
target_sources(cl_test PRIVATE
  ${CLINK_SOURCE_DIR}/src/common/cl_eth.c
  ${CLINK_SOURCE_DIR}/src/common/cl_iefb.c
  ${CLINK_SOURCE_DIR}/src/common/cl_slmp.c
  ${CLINK_SOURCE_DIR}/src/common/cl_slmp_udp.c
  ${CLINK_SOURCE_DIR}/src/common/cl_util.c
  ${CLINK_SOURCE_DIR}/src/common/cl_file.c
  ${CLINK_SOURCE_DIR}/src/master/clm_api.c
  ${CLINK_SOURCE_DIR}/src/master/clm_iefb.c
  ${CLINK_SOURCE_DIR}/src/master/clm_master.c
  ${CLINK_SOURCE_DIR}/src/master/clm_slmp.c
  ${CLINK_SOURCE_DIR}/src/slave/cls_api.c
  ${CLINK_SOURCE_DIR}/src/slave/cls_slave.c
  ${CLINK_SOURCE_DIR}/src/slave/cls_iefb.c
  ${CLINK_SOURCE_DIR}/src/slave/cls_slmp.c
  )

get_target_property(CLINK_OPTIONS clink COMPILE_OPTIONS)
target_compile_options(cl_test PRIVATE
  -DUNIT_TEST
  ${CLINK_OPTIONS}
  )

target_include_directories(cl_test
  PRIVATE
  ${CLINK_SOURCE_DIR}/src
  ${CLINK_BINARY_DIR}/src
  )

# Link against cl target to get the cmake properties for that
# target, such as include paths and libraries.
target_link_libraries(cl_test
  PRIVATE
  clink
  )

# No need for gmock
set(BUILD_GMOCK OFF CACHE BOOL "")
