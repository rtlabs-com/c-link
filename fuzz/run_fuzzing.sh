FUZZER_SETTINGS="-max_len=300 -max_total_time=180"
FUZZER_BUILD_DIR="build.fuzz"

set -e

cmake --build ${FUZZER_BUILD_DIR}

rm -f *.profraw
rm -f fuzz_report.html
rm -f default.profdata

${FUZZER_BUILD_DIR}/cl_fuzz_slave_cyclic fuzz/corpus/slave_cyclic/ ${FUZZER_SETTINGS}
mv default.profraw slave_cyclic.profraw

${FUZZER_BUILD_DIR}/cl_fuzz_slave_slmp fuzz/corpus/slave_slmp/ ${FUZZER_SETTINGS}
mv default.profraw slave_slmp.profraw

${FUZZER_BUILD_DIR}/cl_fuzz_master_cyclic fuzz/corpus/master_cyclic/ ${FUZZER_SETTINGS}
mv default.profraw master_cyclic.profraw

${FUZZER_BUILD_DIR}/cl_fuzz_master_slmp fuzz/corpus/master_slmp/ ${FUZZER_SETTINGS}
mv default.profraw master_slmp.profraw

llvm-profdata-14 merge -sparse *.profraw -o default.profdata

echo ""
echo "Creating HTML report"
llvm-cov-14 show build.fuzz/cl_fuzz_master_slmp \
  -object=build.fuzz/cl_fuzz_master_cyclic \
  -object=build.fuzz/cl_fuzz_slave_slmp \
  -object=build.fuzz/cl_fuzz_slave_cyclic \
  -instr-profile=default.profdata -format=html > fuzz_report.html
