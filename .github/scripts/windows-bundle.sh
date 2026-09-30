#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/../../scripts/env.sh"

PORTABLE_APP_DIR="${DIST_DIR}/${APP_NAME}-${APP_VERSION}-Portable"
mkdir -p "${PORTABLE_APP_DIR}"

BUILD_TYPE="${1:-Release}"
MAKE_INSTALLER="${2:-false}"

export WINDEPLOYQT=$(cygpath -w "${QT_BIN}/windeployqt.exe")

# Single-config generators (Ninja) place the exe at the build root;
# multi-config generators (Visual Studio) place it under ${BUILD_TYPE}/.
EXE_DIR="${BUILD_DIR}/${BUILD_TYPE}"
if [[ ! -f "${EXE_DIR}/${APP_NAME}.exe" ]]; then
  EXE_DIR="${BUILD_DIR}"
fi
if [[ ! -f "${EXE_DIR}/${APP_NAME}.exe" ]]; then
  echo "error: ${APP_NAME}.exe not found under ${BUILD_DIR} or ${BUILD_DIR}/${BUILD_TYPE}" >&2
  exit 1
fi
export EXE_PATH=$(cygpath -w "${EXE_DIR}/${APP_NAME}.exe")

cp "${EXE_PATH}" "${PORTABLE_APP_DIR}/${APP_NAME}.exe"

"${WINDEPLOYQT}" "${PORTABLE_APP_DIR}/${APP_NAME}.exe"

shopt -s nullglob
cp "${EXE_DIR}"/*.dll "${PORTABLE_APP_DIR}/" || true
shopt -u nullglob

"${PORTABLE_APP_DIR}/${APP_NAME}.exe" --smoke-test

if [[ "${MAKE_INSTALLER}" == "false" ]]; then
  exit 0
fi

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
