Compiling for Linux using the clang compiler
--------------------------------------------

This assumes that you are using a Debian based Linux distribution.

#. Install the compiler::

      sudo apt install clang --install-suggests

#. Compile::

      CC=clang CXX=clang++ cmake -B build.clang -S .
      cmake --build build.clang/ -j4

#. Run tests::

      cmake --build build.clang/ --target check
