#!/bin/bash
# Incremental ran_func_rc_test against a prebuilt ran-build:latest image.
# Does not rebuild the full RAN. Overlay only O-RAN sources so the image's
# cmake_targets/ran_build/build stays intact.
#
# Prerequisite:
#   docker build --target ran-build --tag ran-build:latest \
#     --file docker/Dockerfile.build.ubuntu .
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OAI_DIR="$(cd "${SCRIPT_DIR}/../../../../../" && pwd)"
OUT="${OAI_DIR}/../nws/build_scripts/ran_build_test_output.txt"
mkdir -p "$(dirname "${OUT}")"

E2AP_VERSION="${E2AP_VERSION:-E2AP_V3}"
KPM_VERSION="${KPM_VERSION:-KPM_V3_00}"

docker run --rm \
  -v "${OAI_DIR}/openair2/E2AP/RAN_FUNCTION/O-RAN:/oai-ran/openair2/E2AP/RAN_FUNCTION/O-RAN" \
  -v "${OAI_DIR}/openair2/E2AP/flexric/src/sm/rc_sm/enc:/oai-ran/openair2/E2AP/flexric/src/sm/rc_sm/enc" \
  -w /oai-ran \
  ran-build:latest \
  bash -c "
    set -euo pipefail
    . /oai-ran/oaienv
    cd cmake_targets/ran_build/build
    cmake -DENABLE_TESTS=ON \
          -DKPM_VERSION=${KPM_VERSION} \
          -DE2AP_VERSION=${E2AP_VERSION} .
    ninja ran_func_rc_test
    ctest -R ran_func_rc_test -V --output-on-failure
  " 2>&1 | tee "${OUT}"
