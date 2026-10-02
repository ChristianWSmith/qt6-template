#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/../../scripts/env.sh"
# Built-binary resolution (AUD-003/AUD-109): Ninja single-config first.
source "${SCRIPT_DIR}/../../scripts/lib/resolve-app-path.sh"

PORTABLE_APP_DIR="${DIST_DIR}/${APP_NAME}-${APP_VERSION}-Portable"
mkdir -p "${PORTABLE_APP_DIR}"

BUILD_TYPE="${1:-Release}"
MAKE_INSTALLER="${2:-false}"

export WINDEPLOYQT=$(cygpath -w "${QT_BIN}/windeployqt.exe")

if [ "${APP_BIN_EXISTS}" != "1" ]; then
  echo "error: ${APP_NAME}.exe not found (resolved: ${APP_BIN})" >&2
  exit 1
fi

# DLLs sit next to the exe (Conan copy_shared_libs targets the same single-
# config layout the helper resolves).
EXE_DIR="$(dirname "${APP_BIN}")"
export EXE_PATH=$(cygpath -w "${APP_BIN}")

cp "${EXE_PATH}" "${PORTABLE_APP_DIR}/${APP_NAME}.exe"

"${WINDEPLOYQT}" "${PORTABLE_APP_DIR}/${APP_NAME}.exe"

shopt -s nullglob
cp "${EXE_DIR}"/*.dll "${PORTABLE_APP_DIR}/" || true
shopt -u nullglob

"${PORTABLE_APP_DIR}/${APP_NAME}.exe" --smoke-test

if [[ "${MAKE_INSTALLER}" == "false" ]]; then
  exit 0
fi

# AUD-122: emit the generated Inno script under DIST_DIR (not PROJECT_ROOT)
# and clean it up on exit. .gitignore already lists inno.iss at the repo
# root — do not re-add.
mkdir -p "${DIST_DIR}"
ISS_PATH="${DIST_DIR}/inno.iss"
cleanup_iss() {
  rm -f "${ISS_PATH}"
}
trap cleanup_iss EXIT

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

iscc "$(cygpath -w "${ISS_PATH}")"
