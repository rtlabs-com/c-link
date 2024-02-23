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

# Skip warnings:
# C4100: Unreferenced function parameters
# C4127: Expression in if statement is constant (in LOG_ macro)
# C4815: zero-sized array in stack object will have no elements
list(APPEND WARNINGS
  /WX
  /W4
  /wd4100
  /wd4127
  /wd4815
)

target_include_directories(clink
  PRIVATE
  src/ports/windows
  )

target_sources(clink
  PRIVATE
  src/ports/windows/clal_win_utils.c
  src/ports/windows/clal.c
  src/ports/windows/clal_udp.c
  )

target_compile_options(clink
  PRIVATE
  ${WARNINGS}
  /D _CRT_SECURE_NO_WARNINGS
  )

target_link_libraries(clink
  PUBLIC
  iphlpapi
  wsock32
  ws2_32)

target_include_directories(cl_sample_slave
  PRIVATE
  src/ports/windows
  )

target_sources(cl_sample_slave
  PRIVATE
  src/ports/windows/cl_sample_slave.c
  src/ports/windows/clal_win_utils.c
  )

target_compile_options(cl_sample_slave
  PRIVATE
  ${WARNINGS}
  )

target_include_directories(cl_sample_master
  PRIVATE
  src/ports/windows
  )

target_sources(cl_sample_master
  PRIVATE
  src/ports/windows/cl_sample_master.c
  src/ports/windows/clal_win_utils.c
  )

target_compile_options(cl_sample_master
  PRIVATE
  ${WARNINGS}
  )

if (BUILD_TESTING)
  set(GOOGLE_TEST_INDIVIDUAL TRUE)
  target_include_directories(cl_test
    PRIVATE
    src/ports/windows
    )

  target_compile_options(cl_test
    PRIVATE
    ${WARNINGS}
    # Allow skipped test cases ("unreachable code")
    /wd4702
    )
endif()
