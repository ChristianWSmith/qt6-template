#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh


export ARG="${1:-build}"

cleanBuild() {
    rm -rf "${BUILD_DIR}"
    rm -rf "${PROJECT_ROOT}/CMakeUserPresets.json"
}

cleanQt() {
    rm -rf "${QT_INSTALL_DIR}"
    rm -rf "${PROJECT_ROOT}/aqtinstall.log"
}

if [[ "${ARG}" == "all" ]]; then
    cleanBuild
    cleanQt
elif [[ "${ARG}" == "build" ]]; then
    cleanBuild
elif [[ "${ARG}" == "qt" ]]; then
    cleanQt
else
    echo "Usage: $(basename ${0}) <all|build|qt>"
fi

