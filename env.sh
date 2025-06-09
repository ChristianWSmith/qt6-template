#!/usr/bin/env bash
set -euo pipefail

# --- SCRIPT DIR ---
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# APP
export APP_NAME="MyApp"
export APP_VERSION="0.1.0"
export APP_ID="com.example.MyApp"
export APP_CATEGORIES="Utility;"

# --- QT CONFIG ---
export QT_VERSION="6.9.1"

# --- OS DETECTION ---
PLATFORM="$(python3 -c 'import platform; print(platform.system().lower())')"

case "${PLATFORM}" in
  linux)
    export COMPILER_NAME="linux_gcc_64"
    export COMPILER_DIR="gcc_64"
    export AQT_PLATFORM="linux"
    export APP_ICON="${SCRIPT_DIR}/icons/app_icon.png"
    ;;
  windows)
    export COMPILER_NAME="win64_msvc2022_64"
    export COMPILER_DIR="msvc2022_64"
    export AQT_PLATFORM="windows"
    export APP_ICON="$(cygpath -w "${SCRIPT_DIR}/icons/app_icon.ico")"
    ;;
  darwin)
    export COMPILER_NAME="clang_64"
    export COMPILER_DIR="macos"
    export AQT_PLATFORM="mac"
    export APP_ICON="${SCRIPT_DIR}/icons/app_icon.icns"
    ;;
  *)
    echo "Unsupported platform: ${PLATFORM}"
    exit 1
    ;;
esac

# --- DERIVED PATHS ---
export BUILD_DIR="${SCRIPT_DIR}/build"
export QT_ROOT="${SCRIPT_DIR}/Qt/${QT_VERSION}/${COMPILER_DIR}"
export QT_CMAKE_DIR="${QT_ROOT}/lib/cmake/Qt6"
export QT_BIN="${QT_ROOT}/bin"
export QT_PLUGINS_DIR="${QT_ROOT}/plugins"
export QT_PLATFORMS_DIR="${QT_PLUGINS_DIR}/platforms"
export QT_WAYLAND_DIR="${QT_PLUGINS_DIR}/wayland-shell-integration"

if [ "${PLATFORM}" = "windows" ]; then
    export BUILD_DIR="$(cygpath -w "{$BUILD_DIR}")"
    export QT_ROOT="$(cygpath -w "${QT_ROOT}")"
    export QT_CMAKE_DIR="$(cygpath -w "${QT_CMAKE_DIR}")"
    export QT_BIN="$(cygpath -w "${QT_BIN}")"
    export QT_PLUGINS_DIR="$(cygpath -w "${QT_PLUGINS_DIR}")"
    export QT_PLATFORMS_DIR="$(cygpath -w "${QT_PLATFORMS_DIR}")"
    export QT_WAYLAND_DIR="$(cygpath -w "${QT_WAYLAND_DIR}")"
fi