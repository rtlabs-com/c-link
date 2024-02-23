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

list(APPEND WARNINGS
   -Wall
   -Wextra
   -Werror
   -Wno-unused-parameter
  )

target_include_directories(clink
  PRIVATE
  src/ports/rt-kernel
  )

target_sources(clink
  PRIVATE
  src/ports/rt-kernel/clal.c
  src/ports/rt-kernel/clal_udp.c
  )

target_compile_options(clink
  PRIVATE
  ${WARNINGS}
  )

target_link_libraries(clink
  INTERFACE
  $<$<CONFIG:Coverage>:--coverage>
  )

target_include_directories(cl_sample_slave
  PRIVATE
  src/ports/rt-kernel
  )

if (EXISTS ${CLINK_SOURCE_DIR}/src/ports/rt-kernel/cl_sample_${BSP}.c)
  set(BSP_SOURCE src/ports/rt-kernel/cl_sample_${BSP}.c)
else()
  set(BSP_SOURCE src/ports/rt-kernel/cl_sample_bsp.c)
endif()

target_sources(cl_sample_slave
  PRIVATE
  ${BSP_SOURCE}
  src/ports/rt-kernel/cl_sample_slave.c
  )

target_compile_options(cl_sample_slave
  PRIVATE
  ${WARNINGS}
  )

target_include_directories(cl_sample_master
  PRIVATE
  src/ports/rt-kernel
  )

target_sources(cl_sample_master
  PRIVATE
  ${BSP_SOURCE}
  src/ports/rt-kernel/cl_sample_master.c
  )

target_compile_options(cl_sample_master
  PRIVATE
  ${WARNINGS}
  )

if (BUILD_TESTING)
  target_include_directories(cl_test
    PRIVATE
    src/ports/rt-kernel
    )
endif()
