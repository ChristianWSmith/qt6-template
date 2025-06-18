#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/../../scripts/env.sh

mkdir -p "${DIST_DIR}"

export APP_PATH="${BUILD_DIR}/${APP_NAME}.app"
"${QT_BIN}/macdeployqt" "${APP_PATH}"
mv "${APP_PATH}" "${DIST_DIR}/${APP_NAME}.app"
