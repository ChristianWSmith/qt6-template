#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/../../scripts/env.sh"

mkdir -p "${DIST_DIR}"

export APP_PATH="${BUILD_DIR}/${APP_NAME}.app"
"${QT_BIN}/macdeployqt" "${APP_PATH}"

# Custom dylibs handling
FRAMEWORKS_DIR="${APP_PATH}/Contents/Frameworks"
mkdir -p "${FRAMEWORKS_DIR}"

echo "🔍 Copying custom dylibs into app bundle..."
find "${BUILD_DIR}" -maxdepth 1 -name "*.dylib" | while read dylib; do
    base=$(basename "${dylib}")
    cp -v "${dylib}" "${FRAMEWORKS_DIR}/${base}"
done

echo "🛠️ Fixing install names for ${APP_NAME} binary..."
APP_BIN="${APP_PATH}/Contents/MacOS/${APP_NAME}"

# Change linked dylibs in the main binary
for dylib in "${FRAMEWORKS_DIR}"/*.dylib; do
    base=$(basename "${dylib}")
    install_name_tool -change "${BUILD_DIR}/${base}" \
        "@executable_path/../Frameworks/${base}" \
        "${APP_BIN}"
done

echo "🧹 Fixing IDs and nested links inside custom dylibs..."
for dylib in "${FRAMEWORKS_DIR}"/*.dylib; do
    base=$(basename "${dylib}")

    # Set correct install name (so other dylibs can refer to it)
    install_name_tool -id "@rpath/${base}" "${dylib}"

    # Fix links to other custom dylibs (if they depend on each other)
    for other in "${FRAMEWORKS_DIR}"/*.dylib; do
        other_base=$(basename "${other}")
        [[ "${base}" == "${other_base}" ]] && continue

        install_name_tool -change "${BUILD_DIR}/${other_base}" \
            "@rpath/${other_base}" \
            "${dylib}" || true
    done
done

echo "✅ Done fixing dylibs."
mv "${APP_PATH}" "${DIST_DIR}/${APP_NAME}.app"
