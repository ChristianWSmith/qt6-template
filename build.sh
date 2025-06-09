#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

CMAKE_CONFIG="${1:-Release}"

cmake -B "${BUILD_DIR}" -DCMAKE_PREFIX_PATH="${QT_CMAKE_DIR}" -DQT_DEBUG_FIND_PACKAGE=ON
cmake --build "${BUILD_DIR}" --parallel --config "${CMAKE_CONFIG}"
