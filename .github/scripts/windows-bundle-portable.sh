
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/../../scripts/env.sh"

PORTABLE_APP_DIR="${DIST_DIR}/${APP_NAME}-${APP_VERSION}-Portable"
mkdir -p "${PORTABLE_APP_DIR}"

BUILD_TYPE="${1}"

export WINDEPLOYQT=$(cygpath -w "${QT_BIN}/windeployqt.exe")
export EXE_PATH=$(cygpath -w "${BUILD_DIR}/${BUILD_TYPE}/${APP_NAME}.exe")

cp "${EXE_PATH}" "${PORTABLE_APP_DIR}/${APP_NAME}.exe"

"${WINDEPLOYQT}" "${PORTABLE_APP_DIR}/${APP_NAME}.exe"

cp "${BUILD_DIR}/${BUILD_TYPE}"/*.dll "${PORTABLE_APP_DIR}/"
