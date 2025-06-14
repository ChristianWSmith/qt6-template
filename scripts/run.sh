#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

export CMAKE_BUILD_TYPE="${1:-Debug}"

source ${SCRIPT_DIR}/build.sh "${CMAKE_BUILD_TYPE}"

if [ "${PLATFORM}" == "windows" ]; then 
    if [ "$#" -gt 0 ]; then shift; fi
    ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe "${@}"
else
    if [ "$#" -gt 0 ]; then shift; fi
    ${BUILD_DIR}/${APP_NAME} "${@}"
fi