Getting started
===============

In this tutorial you will learn how to clone and build the
software.

Cloning
-------

This project uses a git submodule. You **must** also clone the submodule,
i.e. run git clone like this::

   git clone --recurse-submodules https://github.com/rtlabs-com/c-link.git

If you already cloned the repository without the
``--recurse-submodules`` flag then run this in the c-link folder::

   git submodule update --init --recursive

Using CMake
-----------

This project uses `CMake <https://cmake.org>`_ to generate
cross-platform build systems. CMake must be installed on your host
platform. If you are on Linux it should be available in your package
manager. The Linux-related examples assume that you use a Debian-based
distribution.

::

   sudo apt install cmake

.. note:: Linux examples are for Ubuntu Linux 20.04.

This project requires at least CMake 3.14. If your platform's version
is too old you may install a newer version using Python::

  pip3 install cmake

This should work on all platforms provided you have Python installed.

For Windows, see the :ref:`Installing developer tools on Windows` guide.

.. note:: In the following tutorial and throughout this documentation,
   it is assumed that CMake is available in your path. That is, you
   can run it by just typing ``cmake``.

CMake supports many build systems, e.g. Makefiles or Ninja, or even
IDE projects. The build system should be in a folder separated from
the sources, i.e. an out-of-tree build is recommended. However it is
often convenient to have the build folder in a subdirectory of the
repository. For this reason any subdirectory in the root whose name
begins with ``build`` is ignored by git.

.. note:: In the following tutorial and throughout this documentation,
   unless otherwise specified, ``cmake`` should be run in the root of
   the repository. If you want to run it outside of the repository,
   add the `-S` flag to specify the location of the repository::

     cmake -B build -S /path/to/repository

The general pattern for building a CMake project is to configure and
build in two steps::

  cmake -B build
  cmake --build build/

However, see the following sections for more info on building for some
common platforms. After building, sample applications and test suites
will be in the corresponding build folder.

.. tip::
   Use the `-j` flag for parallel builds::

     cmake --build build -j

The project contains some options that can be adjusted by the
user. ``ccmake`` is a command line utility to interactively set
configuration options. Start it by running::

  ccmake build/

then change options and regenerate the build system by pressing
:kbd:`c` followed by :kbd:`g`. You can also set options by adding them
to the CMake configuration command::

  cmake -B build -DBUILD_TESTING=OFF

.. tip:: If at any time you want to start from scratch you can just
         delete the build folder.

To list all available cmake targets::

  cmake --build build --target help


Building for Linux
------------------
#. Run the following to build the project and to run the unit tests::

       cmake --preset linux
       cmake --build --preset linux
       ctest --preset linux


Setting up a Raspberry Pi to run the sample applications
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#. Make sure your Raspberry Pi is connected to Internet via LAN or WiFi to be
   able to download software.

#. In order to compile c-link on Raspberry Pi, you need a recent version of
   ``cmake``. Install it::

       sudo apt update
       sudo apt install snapd
       sudo reboot
       sudo snap install cmake --classic

#. Verify the installed version::

       cmake --version

#. Compare the installed version with the minimum version required for c-link
   (see the first page).

#. You also need ``git`` to download c-link. Install it using::

      sudo apt install git

Downloading and building C-Link
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#. Create a directory::

       mkdir /home/pi/cclink/
       cd /home/pi/cclink/

#. Clone the source::

       git clone --recurse-submodules https://github.com/rtlabs-com/c-link.git

   This will clone the repository with sub-modules.

#. Create and configure the build (here with LOG_LEVEL set to DEBUG)::

       cmake -B build -S c-link -DBUILD_TESTING=OFF -DLOG_LEVEL=DEBUG

#. Build the code::

       cmake --build build

.. tip:: For embedded Linux, see the `Yocto`_ and `Buildroot`_
   repositories.

.. _Yocto: https://github.com/rtlabs-com/meta-rtlabs
.. _Buildroot: https://github.com/rtlabs-com/br2-rtlabs

Building for Windows
--------------------

Visual Studio 2019 or later is required.

#. Run the following to generate
   a 64-bit Visual Studio project, build it and run the unit tests::

       cmake -B build.x64 -A x64
       cmake --build build.x64 --config Release
       cmake --build build.x64 --config Release --target check

   If you only want to build the sample applications for Windows (and
   skip running the unit tests), then run the following commands (here
   with LOG_LEVEL set to DEBUG)::

       cmake -B build.x64 -A x64 -DBUILD_TESTING=OFF -DLOG_LEVEL=DEBUG
       cmake --build build.x64 --config Release

   .. note:: CMake should find your Visual Studio installation. If you
             have more than one version installed you can select which
             version to use with the `-G` flag::

               cmake -B build.x64 -A x64 -G "Visual Studio 16 2019"

Building for rt-kernel
----------------------

Workbench 2020.1 or later is required. You should use a bash shell,
such as for instance the Command Line in your Toolbox
installation.

A CMake toolchain is used to specify how CMake should build rt-kernel
projects. The toolchain uses the :envvar:`BSP` and :envvar:`RTK`
environment variables to specify the required board and the location
of the rt-kernel tree.

You can choose to create a Workbench project or a standalone
project.

Run this to build a standalone rt-kernel project for the xmc48relax board::

   RTK=/path/to/rt-kernel cmake --preset xmc48relax
   cmake --build --preset xmc48relax

Alternatively, run this to generate a Workbench project that can be imported
to your workspace::

       cd /path/to/workspace
       RTK=/path/to/rt-kernel BSP=xmc48relax cmake \
         -B build.xmc48relax -S /path/to/repository \
         -DCMAKE_TOOLCHAIN_FILE=cmake/tools/toolchain/rt-kernel.cmake \
         -DCMAKE_ECLIPSE_EXECUTABLE=/opt/rt-tools/workbench/Workbench \
         -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE \
         -G "Eclipse CDT4 - Unix Makefiles"

   .. note:: CMake Workbench projects can not include the build
      folder. Therefore the build directory must be located outside of
      the source tree and the `-S` flag must be used to specify the
      location of the sources.

   A source project will be created in the repository root. This project
   can also be imported to Workbench.

#. Use the menu :menuselection:`File --> Import --> General --> Existing Projects`.

#. Click :guilabel:`Browse` and select the build directory that was created earlier.

#. After importing, right-click on the
   project and choose :menuselection:`New --> Convert to a C/C++
   project`. This will setup the project so that the indexer works
   correctly and the Workbench revision control tools can be used.

#. Use the menu :menuselection:`Project --> Build All` to build it.
   The library and the unit tests will be built. Note that the tests
   require a stack of at least 16 kB. You may have to increase
   ``CFG_MAIN_STACK_SIZE`` in your bsp :file:`include/config.h` file.

   For example to modify stack size and to use a static IP address for XMC4800,
   change relevant content of :file:`bsp/xmc48relax/include/config.h` to::

      #define CFG_MAIN_STACK_SIZE 18000

      #undef CFG_LWIP_ADDRESS_DYNAMIC
      #define CFG_LWIP_IPADDR() IP4_ADDR(&ipaddr, 192, 168, 0, 201)
      #define CFG_LWIP_NETMASK() IP4_ADDR(&netmask, 255, 255, 255, 0)
      #define CFG_LWIP_GATEWAY() IP4_ADDR(&gw, 192, 168, 0, 1)
      #define CFG_LWIP_NAMESERVER() IP4_ADDR(&nameserver, 0, 0, 0, 0)

#. To recompile the rt-kernel, run this in the root of the rt-kernel directory::

      source setup.sh xmc48relax
      make -j
