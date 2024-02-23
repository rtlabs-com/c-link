C-Link
======

C-Link is a high-speed CC-Link IE Field Basic master and slave stack. The stack is
written to an OS abstraction layer and can also be used in a bare
metal application. Using the abstraction layer, the stack can run on
Linux, Windows or on an RTOS.

It is easy to use and has a small footprint. It
is especially well suited for embedded systems where resources are
limited and efficiency is crucial.

The stack is supplied with full sources and a sample application.

Also C++ (any version) is supported for application development.

See LICENSE.md for licensing details.

Web resources
-------------

* Source repository: https://github.com/rtlabs-com/c-link

* Documentation: https://rt-labs.com/docs/c-link

* RT-Labs (stack integration, certification services and training): https://rt-labs.com

Getting started with c-link
---------------------------

See :ref:`Getting started`.

Key features
------------

* Both master and slave are implemented

* Easy to use

  * Extensive documentation and instructions on how to get started.
  * Build and run sample application on Raspberry Pi in 30 minutes.

* Portable

  * Written in C.
  * Linux, RTOS, Windows or bare metal.
  * Sources for supported port layers provided.

* Node search

* Remote setting of slave IP address (configurable to allow or not)



Limitations
-----------

- Writing and reading of startup parameters is not supported.


Slave stack
-----------

- Each slave device can support up to 16 occupied stations
- Slave can disable cyclic communication
- Supports sending slave application status, error code and local
  management info to the master.
- Can change IP address at command from master (or disable this
  feature in the config)


Master stack
------------

Master supports:

- Multiple groups (max 64)
- Variable or constant link time
- Up to 64 occupied slave stations (max 16 per group)
- Performing node search
- Setting IP address of slave devices
- Slaves occupying up to 16 stations
- Disabling individual slaves
- Disconnection request from slave station
- Communication statistics and other features suitable when running automated tests of slaves.


Requirements
------------

* CMake 3.14 or later
* A C99-compliant C compiler. The test suite requires C++20.

For Linux:

* GCC 4.6 or later

For rt-kernel:

* Workbench 2020.1 or later

For Windows:

* Visual Studio 19 or later


Dependencies
------------

Some of the platform-dependent parts are located in the OSAL repository and the
cmake-tools repository.

* https://github.com/rtlabs-com/osal
* https://github.com/rtlabs-com/cmake-tools

Those are downloaded automatically during install.

The c-link stack contains no third party components. Its external dependencies are:

* C-library
* An operating system (if used)

Tools used for building, testing and documentation (not shipped in the resulting binaries):

* cmake (BSD 3-clause License) https://cmake.org
* gtest (BSD 3-clause License) https://github.com/google/googletest
* Sphinx (BSD license) https://www.sphinx-doc.org
* Doxygen (GPL v2) https://www.doxygen.nl
* clang-format (Apache License 2.0) https://clang.llvm.org/docs/ClangFormat.html


Contributions
-------------

Contributions are welcome. If you want to contribute you will need to
sign a Contributor License Agreement and send it to us either by
e-mail or by physical mail. More information is available at https://rt-labs.com/contribution.
