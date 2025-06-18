#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/../../scripts/env.sh

mkdir -p "${DIST_DIR}"

BUILD_TYPE="${1}"

export WINDEPLOYQT=$(cygpath -w "${QT_BIN}/windeployqt.exe")
export EXE_PATH=$(cygpath -w "${BUILD_DIR}/${BUILD_TYPE}/${APP_NAME}.exe")

"${WINDEPLOYQT}" "${EXE_PATH}"

export ISS_PATH=$(cygpath -w "${PROJECT_ROOT}/inno.iss")
cat > "${ISS_PATH}" <<EOF
[Setup]
AppName=${APP_NAME}
AppVersion=${APP_VERSION}
DefaultDirName={commonpf}\\${APP_NAME}
OutputDir=${BUILD_DIR}
OutputBaseFilename=${APP_NAME}Installer
Compression=lzma
SolidCompression=yes
SetupIconFile=${APP_ICON}

[Files]
Source: "${BUILD_DIR}\\${BUILD_TYPE}\\${APP_NAME}.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "${BUILD_DIR}\\${BUILD_TYPE}\\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
Source: "${BUILD_DIR}\\${BUILD_TYPE}\\platforms\\*"; DestDir: "{app}\\platforms"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\\${APP_NAME}"; Filename: "{app}\\${APP_NAME}.exe"
Name: "{commondesktop}\\${APP_NAME}"; Filename: "{app}\\${APP_NAME}.exe"
EOF

iscc "${ISS_PATH}"

mv "${BUILD_DIR}/${APP_NAME}Installer.exe" "${DIST_DIR}/${APP_NAME}-${APP_VERSION}-Setup.exe"
