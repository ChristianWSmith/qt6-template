#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

export CMAKE_BUILD_TYPE="${1:-Debug}"
export BUILD_TESTING="${2:-OFF}"
export UPDATE_TRANSLATIONS="${3:-OFF}"

source ${SCRIPT_DIR}/build.sh "${CMAKE_BUILD_TYPE}" "${BUILD_TESTING}"

if [ "${PLATFORM}" == "windows" ]; then 
    if [ "$#" -gt 0 ]; then shift; fi
    if [ "$#" -gt 0 ]; then shift; fi
    if [ "$#" -gt 0 ]; then shift; fi
    ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe "${@}"
else
    if [ "$#" -gt 0 ]; then shift; fi
    if [ "$#" -gt 0 ]; then shift; fi
    if [ "$#" -gt 0 ]; then shift; fi
    ${BUILD_DIR}/${APP_NAME} "${@}"
fi