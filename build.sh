#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

CMAKE_CONFIG="${1:-Release}"

pipenv run conan install . \
  --output-folder="${BUILD_DIR}" \
  --build=missing \
  --profile:build="${CONAN_PROFILE}" \
  --profile:host="${CONAN_PROFILE}"

if [[ "${PLATFORM}" == "windows" ]]; then
  VCVARSALL="C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Auxiliary\\Build\\vcvarsall.bat"

  # Escape backslashes for cmd.exe usage
  VCVARSALL_ESCAPED=$(echo "${VCVARSALL}" | sed 's|\\|\\\\|g')

  # Create a temporary batch file to execute properly in CMD
  WRAPPER_BAT="$(mktemp --suffix=.bat)"

  cat > "${WRAPPER_BAT}" <<EOF
@echo off
call "${VCVARSALL_ESCAPED}" amd64
if errorlevel 1 exit /b %errorlevel%
cmake -B "${BUILD_DIR}" ^
  -DCMAKE_TOOLCHAIN_FILE="${CONAN_TOOLCHAIN}" ^
  -DCMAKE_PREFIX_PATH="${QT_CMAKE_DIR}" ^
  -DCMAKE_BUILD_TYPE=${CMAKE_CONFIG} ^
  -DQT_DEBUG_FIND_PACKAGE=ON
if errorlevel 1 exit /b %errorlevel%
cmake --build "${BUILD_DIR}" --parallel --config "${CMAKE_CONFIG}"
exit /b %errorlevel%
EOF

    echo "Created build wrapper: ${WRAPPER_BAT}"
    cat "${WRAPPER_BAT}"

    # Execute it from bash
    cmd.exe /c "${WRAPPER_BAT}"
else
  cmake -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${CONAN_TOOLCHAIN}" \
    -DCMAKE_PREFIX_PATH="${QT_CMAKE_DIR}" \
    -DCMAKE_BUILD_TYPE=${CMAKE_CONFIG} \
    -DQT_DEBUG_FIND_PACKAGE=ON
  cmake --build "${BUILD_DIR}" --parallel --config ${CMAKE_CONFIG}
fi

