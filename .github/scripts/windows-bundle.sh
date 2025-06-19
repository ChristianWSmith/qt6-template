
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

shopt -s nullglob
cp "${BUILD_DIR}/${BUILD_TYPE}"/*.dll "${PORTABLE_APP_DIR}/" || true
shopt -u nullglob

"${PORTABLE_APP_DIR}/${APP_NAME}.exe" --smoke-test

export ISS_PATH=$(cygpath -w "${PROJECT_ROOT}/inno.iss")
cat > "${ISS_PATH}" <<EOF
[Setup]
AppName=${APP_NAME}
AppVersion=${APP_VERSION}
DefaultDirName={commonpf}\\${APP_NAME}
OutputDir=${DIST_DIR}
OutputBaseFilename=${APP_NAME}-${APP_VERSION}-Setup
Compression=lzma
SolidCompression=yes
SetupIconFile=${APP_ICON}

[Files]
Source: "${PORTABLE_APP_DIR}\\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\\${APP_NAME}"; Filename: "{app}\\${APP_NAME}.exe"
Name: "{commondesktop}\\${APP_NAME}"; Filename: "{app}\\${APP_NAME}.exe"
EOF

iscc "${ISS_PATH}"
