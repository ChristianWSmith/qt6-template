
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/../../scripts/env.sh"

PORTABLE_APP_DIR="${DIST_DIR}/${APP_NAME}-${APP_VERSION}-Portable"
mkdir -p "${PORTABLE_APP_DIR}"

BUILD_TYPE="${1}"

export WINDEPLOYQT=$(cygpath -w "${QT_BIN}/windeployqt.exe")
export EXE_PATH=$(cygpath -w "${BUILD_DIR}/${BUILD_TYPE}/${APP_NAME}.exe")

"${WINDEPLOYQT}" "${EXE_PATH}"

cp "${BUILD_DIR}/${BUILD_TYPE}/${APP_NAME}.exe" "${PORTABLE_APP_DIR}/"
cp "${BUILD_DIR}/${BUILD_TYPE}"/*.dll "${PORTABLE_APP_DIR}/"
cp -r "${BUILD_DIR}/${BUILD_TYPE}/platforms" "${PORTABLE_APP_DIR}/"
