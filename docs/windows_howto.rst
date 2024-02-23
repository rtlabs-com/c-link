Using c-link stack on Windows
=============================

Installing developer tools on Windows
-------------------------------------
#. Install the Chocolatey package manager for Window, by following the
   instructions on https://chocolatey.org/install

#. Install the community edition of Visual Studio and other developer tools,
   by running this in an administrative PowerShell::

     choco install -y visualstudio2019community
     choco install -y visualstudio2019-workload-nativedesktop
     choco install -y git
     choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System'

#. If you would like to install the Visual Studio Code editor, run this::

     choco install -y vscode

   Visual Studio and Git Bash will automatically be added to the Windows start menu.
   Use the Git Bash terminal when cloning the c-link repository.

   Alternatively you can install cmake using the Python ``pip3`` tool, as
   described in the :ref:`Getting started` guide.

   If you instead would like to install CMake manually, you can
   download it from https://cmake.org/download.


Adjusting cmake settings on Windows
-----------------------------------
#. To modify the cmake settings, use a PowerShell in the root of
   your repository and run::

       cmake-gui

#. Make sure that the path in the :guilabel:`Where is the source code` text box
   points to the :file:`c-link` directory,
   and the path in :guilabel:`Where to build the binaries` points to the
   :file:`build.x64` directory.

#. Adjust the settings, for example the log level.

#. Press :guilabel:`Configure` and :guilabel:`Generate`.

#. Recompile the application from the command line.


Running the tests on Windows
----------------------------
#. In the directory :file:`build.x64\\Release` run::

       .\cl_test.exe

   or to run a single test case::

       .\cl_test.exe --gtest_filter=UtilUnitTest.UtilIpToString
