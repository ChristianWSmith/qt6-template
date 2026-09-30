#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/../../scripts/env.sh"

mkdir -p "${DIST_DIR}"

export APP_PATH="${BUILD_DIR}/${APP_NAME}.app"
"${QT_BIN}/macdeployqt" "${APP_PATH}"

FRAMEWORKS_DIR="${APP_PATH}/Contents/Frameworks"
mkdir -p "${FRAMEWORKS_DIR}"

APP_BIN="${APP_PATH}/Contents/MacOS/${APP_NAME}"

# Repath any non-Qt shared libraries Conan dropped next to the binary.
# Conan deps are often static — skip cleanly when there are none.
shopt -s nullglob
build_dylibs=("${BUILD_DIR}"/*.dylib)
if [[ ${#build_dylibs[@]} -gt 0 ]]; then
    for dylib in "${build_dylibs[@]}"; do
        base=$(basename "${dylib}")
        cp -v "${dylib}" "${FRAMEWORKS_DIR}/${base}"
    done

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
fi
shopt -u nullglob

# install_name_tool invalidates the code signature; ad-hoc re-sign so the
# bundle still launches.
codesign --force --deep -s - "${APP_PATH}"

mv "${APP_PATH}" "${DIST_DIR}/${APP_NAME}.app"

QT_QPA_PLATFORM=offscreen \
    "${DIST_DIR}/${APP_NAME}.app/Contents/MacOS/${APP_NAME}" --smoke-test
