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

target_sources(cl_sample_slave
  PRIVATE
  cl_sample_slave_common.c
  )

target_include_directories(cl_sample_slave
  PRIVATE
  ${CLINK_SOURCE_DIR}/sample_apps
  )

set_target_properties (cl_sample_slave
  PROPERTIES
  C_STANDARD 99
  )

target_link_libraries (cl_sample_slave PUBLIC clink)

install (TARGETS cl_sample_slave DESTINATION bin)

target_sources(cl_sample_master
  PRIVATE
  cl_sample_master_common.c
  )

target_include_directories(cl_sample_master
  PRIVATE
  ${CLINK_SOURCE_DIR}/sample_apps
  )

set_target_properties (cl_sample_master
  PROPERTIES
  C_STANDARD 99
  )

target_link_libraries (cl_sample_master PUBLIC clink)

install (TARGETS cl_sample_master DESTINATION bin)
