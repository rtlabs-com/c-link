Running static analysis
-----------------------

To install a more recent version of clang-tidy than what is available
in your Linux distribution, see https://apt.llvm.org/

#. Run the clang-tidy static analysis tool::

     cmake -B build
     run-clang-tidy-16 -p build/ cl[ams_]

   where the last argument is a regular expression describing which files
   should be analyzed.

   There should be no warnings or errors when running the command. Note that there must be a
   :file:`compile_commands.json` file available in the :file:`build/` directory.

#. Similarly, to run the static analysis tool on the test files::

     run-clang-tidy -p build/ /test/

   To run the analysis on a single source file::

      clang-tidy -p build/ test/test_slave_iefb.cpp

.. note:: ``run-clang-tidy`` is a script that is used to run several instances of ``clang-tidy`` in parallel.
