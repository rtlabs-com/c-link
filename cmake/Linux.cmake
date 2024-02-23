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
  $<$<C_COMPILER_ID:Clang>:-Wconversion -Wno-sign-conversion>
  )

##### C-link library
target_include_directories(clink
  PRIVATE
  src/ports/linux
  )

target_sources(clink
  PRIVATE
  src/ports/linux/clal.c
  src/ports/linux/clal_udp.c
  src/ports/linux/clal_filetools.c
  )

target_compile_options(clink
  PRIVATE
  ${WARNINGS}
  INTERFACE
  $<$<CONFIG:Coverage>:--coverage>
  )

target_link_libraries(clink
  INTERFACE
  $<$<CONFIG:Coverage>:--coverage>
  )

##### Sample slave
if (CMAKE_PROJECT_NAME STREQUAL CLINK AND NOT BUILD_FUZZ)
  target_include_directories(cl_sample_slave
    PRIVATE
    src/ports/linux
    )

  target_sources(cl_sample_slave
    PRIVATE
    src/ports/linux/cl_sample_slave.c
    )

  target_compile_options(cl_sample_slave
    PRIVATE
    ${WARNINGS}
    )
endif()

##### Sample master
if (CMAKE_PROJECT_NAME STREQUAL CLINK AND NOT BUILD_FUZZ)
  target_include_directories(cl_sample_master
    PRIVATE
    src/ports/linux
    )

  target_sources(cl_sample_master
    PRIVATE
    src/ports/linux/cl_sample_master.c
    )

  target_compile_options(cl_sample_master
    PRIVATE
    ${WARNINGS}
    )
endif()

##### Sample combined slave and master
if (CMAKE_PROJECT_NAME STREQUAL CLINK AND NOT BUILD_FUZZ)
  add_executable(cl_sample_combined "")

  set_target_properties (cl_sample_combined
    PROPERTIES
    C_STANDARD 99
    )

  target_link_libraries (cl_sample_combined PUBLIC clink)

  install (TARGETS cl_sample_combined DESTINATION bin)

  target_include_directories(cl_sample_combined
    PRIVATE
    src/ports/linux
    sample_apps
    )

  target_sources(cl_sample_combined
    PRIVATE
    src/ports/linux/cl_sample_combined.c
    sample_apps/cl_sample_master_common.c
    sample_apps/cl_sample_slave_common.c
    )

  target_compile_options(cl_sample_combined
    PRIVATE
    ${WARNINGS}
    )
endif()

##### Testing
if (BUILD_TESTING AND NOT BUILD_FUZZ)
  set(GOOGLE_TEST_INDIVIDUAL TRUE)
  target_include_directories(cl_test
    PRIVATE
    src/ports/linux
    )
endif()

##### Fuzzing
if (BUILD_FUZZ)
  target_include_directories(cl_fuzz_slave_cyclic
    PRIVATE
    src/ports/linux
    )
  target_include_directories(cl_fuzz_slave_slmp
    PRIVATE
    src/ports/linux
    )
  target_include_directories(cl_fuzz_master_cyclic
    PRIVATE
    src/ports/linux
    )
  target_include_directories(cl_fuzz_master_slmp
    PRIVATE
    src/ports/linux
    )
endif()


##### Networking scripts
file(COPY
  src/ports/linux/set_ip_address_netmask
  src/ports/linux/set_cclink_leds
  src/ports/linux/set_cclink_leds.raspberrypi
  DESTINATION
  ${CLINK_BINARY_DIR}/
  )
install (PROGRAMS
  src/ports/linux/set_ip_address_netmask
  src/ports/linux/set_cclink_leds
  src/ports/linux/set_cclink_leds.raspberrypi
  DESTINATION bin)
