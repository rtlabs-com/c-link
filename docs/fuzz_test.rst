Fuzz test
---------

Build fuzz test binaries using `libFuzzer
<https://llvm.org/docs/LibFuzzer.html>`_::

  CC=clang CXX=clang++ cmake -B build.fuzz -DBUILD_FUZZ=ON -DCMAKE_BUILD_TYPE=Debug
  cmake --build build.fuzz

libFuzzer is a coverage-guided fuzzer. It generates random input data
using code coverage as a guide for how to mutate the data. It runs
until it finds a fault with the tested code.

Run a fuzz test for incoming cyclic data in slave::

  build.fuzz/cl_fuzz_slave_cyclic fuzz/corpus/slave_cyclic/ -max_len=300 -max_total_time=180

A file ``default.profraw`` will be created in the current directory.

To display the code covered by the tests::

  llvm-profdata-14 merge -sparse *.profraw -o default.profdata
  llvm-cov-14 show build.fuzz/cl_fuzz_slave_cyclic -instr-profile=default.profdata -format=html > fuzz_report.html

To show a summary of coverage per file::

  llvm-cov-14 report build.fuzz/cl_fuzz_slave_cyclic -instr-profile=default.profdata

Similarly for the SLMP communication in the slave::

  build.fuzz/cl_fuzz_slave_slmp fuzz/corpus/slave_slmp/ -max_len=100 -max_total_time=180
  llvm-profdata-14 merge -sparse *.profraw -o default.profdata
  llvm-cov-14 show build.fuzz/cl_fuzz_slave_slmp -instr-profile=default.profdata -format=html > fuzz_report.html

For master cyclic communication::

   build.fuzz/cl_fuzz_master_cyclic fuzz/corpus/master_cyclic/ -max_len=300 -max_total_time=180
   llvm-profdata-14 merge -sparse *.profraw -o default.profdata
   llvm-cov-14 show build.fuzz/cl_fuzz_master_cyclic -instr-profile=default.profdata -format=html > fuzz_report.html

For master SLMP communication::

   build.fuzz/cl_fuzz_master_slmp fuzz/corpus/master_slmp/ -max_len=100 -max_total_time=180
   llvm-profdata-14 merge -sparse *.profraw -o default.profdata
   llvm-cov-14 show build.fuzz/cl_fuzz_master_slmp -instr-profile=default.profdata -format=html > fuzz_report.html


Combining coverage from several fuzz tests into a single report
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#. After each of the four runs of the fuzz test binaries, rename the output
   file so it not will be overwritten by a subsequent run. For example::

      mv default.profraw slave_cyclic.profraw

#. Merge all the coverage files::

      llvm-profdata-14 merge -sparse *.profraw -o default.profdata

#. Generate the report::

      llvm-cov-14 show build.fuzz/cl_fuzz_master_slmp \
        -object=build.fuzz/cl_fuzz_master_cyclic \
        -object=build.fuzz/cl_fuzz_slave_slmp \
        -object=build.fuzz/cl_fuzz_slave_cyclic \
        -instr-profile=default.profdata -format=html > fuzz_report.html

There is a script for running all four fuzzers, and combine the result into a
HTML report. After you have created the relevant build directory, run::

   fuzz/run_fuzzing.sh


Extracting seed data for fuzzing from Wireshark recordings
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#. To export the UDP payload of a frame to file, use the "Packet details" view.

#. On the line for example "CC-Link IE Field Basic, Request" right-click and
   select "Export Packet Bytes ...".

#. Give it a filename and click Save.

#. Verify that the file contains the correct number of bytes.
