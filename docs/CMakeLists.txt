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

cmake_policy(SET CMP0057 NEW)

find_package(Doxygen)
find_package(Sphinx)

if (DOXYGEN_FOUND)

  # Doxygen configuration
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
  set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_STRIP_CODE_COMMENTS NO)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
  set(DOXYGEN_PREDEFINED CL_EXPORT)
  set(DOXYGEN_EXPAND_AS_DEFINED CL_EXPORT)
  set(DOXYGEN_COLLABORATION_GRAPH NO)
  set(DOXYGEN_INCLUDE_GRAPH NO)
  set(DOXYGEN_INCLUDED_BY_GRAPH NO)
  set(DOXYGEN_RECURSIVE NO)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  set(DOXYGEN_GENERATE_XML YES)
  set(DOXYGEN_WARN_AS_ERROR FAIL_ON_WARNINGS) # Doxygen 1.9+
  set(DOXYGEN_ALIASES
    [[req="\xrefitem req \"Requirement\" \"Requirements\" "]]
    )
  set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES)
  doxygen_add_docs(doxygen
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    USE_STAMP_FILE
    README.md
    include/cl_common.h
    include/clm_api.h
    include/cls_api.h
    src/common/cl_eth.c
    src/common/cl_eth.h
    src/common/cl_file.c
    src/common/cl_file.h
    src/common/cl_iefb.c
    src/common/cl_iefb.h
    src/common/cl_limiter.c
    src/common/cl_limiter.h
    src/common/cl_literals.c
    src/common/cl_literals.h
    src/common/cl_slmp_udp.c
    src/common/cl_slmp_udp.h
    src/common/cl_slmp.c
    src/common/cl_slmp.h
    src/common/cl_timer.c
    src/common/cl_timer.h
    src/common/cl_types.h
    src/common/cl_util.c
    src/common/cl_util.h
    src/master/clm_api.c
    src/master/clm_iefb.c
    src/master/clm_iefb.h
    src/master/clm_master.c
    src/master/clm_master.h
    src/master/clm_slmp.c
    src/master/clm_slmp.h
    src/slave/cls_api.c
    src/slave/cls_iefb.c
    src/slave/cls_iefb.h
    src/slave/cls_slave.c
    src/slave/cls_slave.h
    src/slave/cls_slmp.c
    src/slave/cls_slmp.h
    test/test_both_master_slave.cpp
    test/test_common_eth.cpp
    test/test_common_file.cpp
    test/test_common_iefb.cpp
    test/test_common_limiter.cpp
    test/test_common_literals.cpp
    test/test_common_slmp_udp.cpp
    test/test_common_slmp.cpp
    test/test_common_timer.cpp
    test/test_common_util.cpp
    test/test_master_api.cpp
    test/test_master_iefb.cpp
    test/test_master_slmp.cpp
    test/test_master.cpp
    test/test_memory_functions.cpp
    test/test_slave_api.cpp
    test/test_slave_iefb.cpp
    test/test_slave_slmp.cpp
    test/test_slave.cpp
    )
endif()

if (SPHINX_FOUND AND DOXYGEN_FOUND)
  set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
  set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

  # Targets for Sphinx html, spell checking and link checking
  macro(add_sphinx_builder)
    add_custom_target(${ARGV0}
      COMMAND
      ${SPHINX_EXECUTABLE} -M ${ARGV1}
      ${SPHINX_SOURCE} ${SPHINX_BUILD}
      -a -E -W --keep-going
      -Dbreathe_projects.cl=${CMAKE_CURRENT_BINARY_DIR}/xml
      COMMENT "Running sphinx-build ${ARGV1}"
    )
    add_dependencies(${ARGV0} doxygen)
  endmacro()

  add_sphinx_builder(sphinx-html html)
  add_sphinx_builder(sphinx-pdf latexpdf)
  add_sphinx_builder(sphinx-spelling spelling)
  add_sphinx_builder(sphinx-linkcheck linkcheck)

  # Create Graphviz dot files
  add_custom_target(
    dotfiles ALL
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR}/sphinx/html/_images/
    COMMAND python3 ${SPHINX_SOURCE}/parse_statemachine.py
    BYPRODUCTS ${SPHINX_SOURCE}/_generated/slave_state_machine.dot
    BYPRODUCTS ${SPHINX_SOURCE}/_generated/device_state_machine.dot
    BYPRODUCTS ${SPHINX_SOURCE}/_generated/group_state_machine.dot
    COMMENT "Creating dot files for Sphinx Graphviz figures")
  add_dependencies(sphinx-html dotfiles)
  add_dependencies(sphinx-pdf dotfiles)

  # Create requirements report
  add_custom_target(
    requirement_report ALL
    COMMAND python3 ${SPHINX_SOURCE}/parse_requirements.py ${CMAKE_CURRENT_BINARY_DIR}/xml
    BYPRODUCTS ${SPHINX_SOURCE}/_generated/req_testlocation_report.rst
    BYPRODUCTS ${SPHINX_SOURCE}/_generated/specification_report.rst
    COMMENT "Creating requirement report")
  add_dependencies(requirement_report doxygen)
  add_dependencies(sphinx-html requirement_report)
  add_dependencies(sphinx-pdf requirement_report)

  # Save sample application helptexts
  add_custom_target(
    sampleapp_helptexts ALL
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../cl_sample_master -h > ${SPHINX_SOURCE}/_generated/helptext_samplemaster.txt || true
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/../cl_sample_slave -h  > ${SPHINX_SOURCE}/_generated/helptext_sampleslave.txt || true
    BYPRODUCTS ${SPHINX_SOURCE}/_generated/helptext_sampleslave.txt
    BYPRODUCTS ${SPHINX_SOURCE}/_generated/helptext_samplemaster.txt
    COMMENT "Saving sampleapp helptexts")
  add_dependencies(sampleapp_helptexts cl_sample_slave)
  add_dependencies(sampleapp_helptexts cl_sample_master)
  add_dependencies(sphinx-html sampleapp_helptexts)
  add_dependencies(sphinx-pdf sampleapp_helptexts)

  # Zip the documentation
  message("c-link revision: ${CLINK_GIT_REVISION}")
  add_custom_command(
    TARGET sphinx-html POST_BUILD
    COMMAND zip -FS -r clink_docs_${CLINK_GIT_REVISION}.zip sphinx/html/ > /dev/null
    COMMAND echo "The Sphinx HTML and ZIP files are located in ${CMAKE_CURRENT_BINARY_DIR}"
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/clink_docs_${CLINK_GIT_REVISION}.zip
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Zipping HTML documentation")

  # Copy and rename PDF file
  add_custom_command(
    TARGET sphinx-pdf POST_BUILD
    COMMAND cp sphinx/latex/c-link.pdf ./c-link_${CLINK_GIT_REVISION}.pdf
    COMMAND echo "The Sphinx PDF is located in ${CMAKE_CURRENT_BINARY_DIR}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying PDF documentation")
endif()
