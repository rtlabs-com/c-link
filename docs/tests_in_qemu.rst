Running tests in the QEMU emulator
==================================

Testing on emulated ARM architecture
------------------------------------
Use the "integrator" BSP for rt-kernel in order to run tests on emulated ARM architecture.

#. Install the ARM cross compiler and QEMU emulator::

      sudo apt-get install rt-collab-arm-eabi-gcc rt-collab-qemu-system-arm

#. Build the rt-kernel for emulated ARM (by using the BSP "integrator")::

      source setup.sh integrator
      make -j

#. Build the cc-link stack and tests::

      RTK=/path/to/rt-kernel BSP=integrator cmake \
        -B build.integrator \
        -DCMAKE_TOOLCHAIN_FILE=cmake/tools/toolchain/rt-kernel.cmake
      cmake --build build.integrator

   With a prebuilt rt-kernel you should typically use ``RTK=/opt/rt-tools/rt-kernel-arm9e``.

#. Run tests::

      /opt/rt-tools/qemu/bin/qemu-system-arm -M integratorcp -nographic \
        -semihosting -kernel build.integrator/cl_test.elf


Testing on emulated PPC architecture
------------------------------------
The c-link CCIEFB stack is intended to be useful on both little-endian and
big-endian platforms. Therefore it is useful to run the tests not only on
the more common little-endian platforms, but also on the PPC architecture
which is big-endian.

#. Install the PowerPC cross compiler and QEMU emulator::

      sudo apt-get install rt-collab-powerpc-eabi-gcc rt-collab-qemu-system-ppc

#. Build the rt-kernel for PowerPC (by using the BSP "prep")::

      source setup.sh prep
      make -j

#. Build the cc-link stack and tests::

      RTK=/path/to/rt-kernel BSP=prep cmake \
        -B build.prep \
        -DCMAKE_TOOLCHAIN_FILE=cmake/tools/toolchain/rt-kernel.cmake \
        -G "Unix Makefiles"
      cmake --build build.prep

#. Start QEMU (halted)::

      /opt/rt-tools/qemu/bin/qemu-system-ppc -M 40p -nographic \
        -L /opt/rt-tools/qemu/share/qemu -s -S

#. Start gdb in another terminal::

      /opt/rt-tools/compilers/powerpc-eabi/bin/powerpc-eabi-gdb \
        build.prep/cl_test.elf -ex "target extended-remote :1234" \
        -ex "load" -ex "tui enable"

#. Run the tests by entering ``c`` and press RETURN in gdb. The output will appear
   in the QEMU terminal.

Quit QEMU by pressing CTRL-A X (not CTRL-A CTRL-X). Quit gdb by entering ``quit``.
