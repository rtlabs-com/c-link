Running tests on Linux
======================

Running tests
-------------

#. Use ccmake to set ``BUILD_TESTING`` to ``ON`` and ``CMAKE_BUILD_TYPE`` to ``Coverage``::

      ccmake build/

#. Run tests::

      cmake --build build/ --target all check

   List available test cases::

      build/cl_test --gtest_list_tests

   Run a single test case::

      build/cl_test --gtest_filter=IefbUnitTest.CciefbCalculateCyclicRequestSize

   Run tests matching a pattern::

      build/cl_test --gtest_filter=IefbUnitTest*


Measuring code coverage
------------------------

#. Create a test coverage report::

      cmake --build build/ --target coverage-report

#. The resulting report is found at build/coverage.html

   Coverage data is stored in .gcda files during execution. They will be removed
   (among other files) when running::

      cmake --build build/ --target clean


Scan the network from Linux
---------------------------

To find connected devices on your subnet, use the Linux tool ``arp-scan``.
It uses ARP requests to find responding devices. Run the command::

   sudo arp-scan -l --interface enp0s31f6

The output looks like this::

   Interface: enp0s31f6, type: EN10MB, MAC: 54:ee:75:ab:cd:ef, IPv4: 192.168.0.200
   Starting arp-scan 1.9.7 with 256 hosts (https://github.com/royhills/arp-scan)
   192.168.0.201       28:e9:8e:2f:e4:b7       Mitsubishi Electric Corporation

   1 packets received by filter, 0 packets dropped by kernel
   Ending arp-scan 1.9.7: 256 hosts scanned in 2.043 seconds (125.31 hosts/sec). 1 responded
