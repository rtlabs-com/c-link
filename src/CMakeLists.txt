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

# NOTE: Add headers to make them show up in an IDE
target_sources (clink PRIVATE
  ${CLINK_SOURCE_DIR}/include/cl_common.h
  ${CLINK_SOURCE_DIR}/include/clm_api.h
  ${CLINK_SOURCE_DIR}/include/cls_api.h
  common/cl_eth.c
  common/cl_eth.h
  common/cl_file.c
  common/cl_file.h
  common/cl_iefb.c
  common/cl_iefb.h
  common/cl_limiter.c
  common/cl_limiter.h
  common/cl_literals.c
  common/cl_literals.h
  common/cl_slmp_udp.c
  common/cl_slmp_udp.h
  common/cl_slmp.c
  common/cl_slmp.h
  common/cl_timer.c
  common/cl_timer.h
  common/cl_types.h
  common/cl_util.c
  common/cl_util.h
  master/clm_api.c
  master/clm_iefb.c
  master/clm_iefb.h
  master/clm_master.c
  master/clm_master.h
  master/clm_slmp.c
  master/clm_slmp.h
  slave/cls_api.c
  slave/cls_iefb.c
  slave/cls_iefb.h
  slave/cls_slave.c
  slave/cls_slave.h
  slave/cls_slmp.c
  slave/cls_slmp.h
  ${CLINK_SOURCE_DIR}/test/mocks.h
  ${CLINK_SOURCE_DIR}/test/mocks.cpp
  )
