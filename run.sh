#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

export CMAKE_BUILD_TYPE="${1:-Release}"

source ${PROJECT_ROOT}/build.sh "${CMAKE_BUILD_TYPE}"

if [ "${PLATFORM}" == "windows" ]; then 
    ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe
else
    ${BUILD_DIR}/${APP_NAME}
fi