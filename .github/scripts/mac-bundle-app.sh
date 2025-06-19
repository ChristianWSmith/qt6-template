#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/../../scripts/env.sh"

mkdir -p "${DIST_DIR}"

export APP_PATH="${BUILD_DIR}/${APP_NAME}.app"
"${QT_BIN}/macdeployqt" "${APP_PATH}"

FRAMEWORKS_DIR="${APP_PATH}/Contents/Frameworks"
mkdir -p "${FRAMEWORKS_DIR}"

find "${BUILD_DIR}" -maxdepth 1 -name "*.dylib" | while read dylib; do
    base=$(basename "${dylib}")
    cp -v "${dylib}" "${FRAMEWORKS_DIR}/${base}"
done

APP_BIN="${APP_PATH}/Contents/MacOS/${APP_NAME}"

for dylib in "${FRAMEWORKS_DIR}"/*.dylib; do
    base=$(basename "${dylib}")
    install_name_tool -change "${BUILD_DIR}/${base}" \
        "@executable_path/../Frameworks/${base}" \
        "${APP_BIN}"
done

for dylib in "${FRAMEWORKS_DIR}"/*.dylib; do
    base=$(basename "${dylib}")

    install_name_tool -id "@rpath/${base}" "${dylib}"

    for other in "${FRAMEWORKS_DIR}"/*.dylib; do
        other_base=$(basename "${other}")
        [[ "${base}" == "${other_base}" ]] && continue

        install_name_tool -change "${BUILD_DIR}/${other_base}" \
            "@rpath/${other_base}" \
            "${dylib}" || true
    done
done

mv "${APP_PATH}" "${DIST_DIR}/${APP_NAME}.app"

open -n "${DIST_DIR}/${APP_NAME}.app" --args "--smoke-test"
