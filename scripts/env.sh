#!/usr/bin/env bash
set -euo pipefail

ENV_SOURCED="${ENV_SOURCED:-}"
if [ ! -z "${ENV_SOURCED}" ]; then 
  return 0
fi
export ENV_SOURCED=1

installPipenv() {
  PIPENV_INSTALLED="${PIPENV_INSTALLED:-}"
  if [ ! -z "${PIPENV_INSTALLED}" ]; then 
    return 0
  fi
  export PIPENV_INSTALLED=1
  pipenv install --dev
}

# --- SCRIPT DIR ---
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
export PROJECT_ROOT="${SCRIPT_DIR}/.."

# --- APP/QT ---
source "${PROJECT_ROOT}/app.env"

# --- OS DETECTION ---
export PLATFORM="$(python3 -c 'import platform; print(platform.system().lower())')"

case "${PLATFORM}" in
  linux)
    export COMPILER_NAME="linux_gcc_64"
    export COMPILER_DIR="gcc_64"
    export AQT_PLATFORM="linux"
    export APP_ICON="${PROJECT_ROOT}/resources/icons/app_icon.png"
    export QT_MODULES="qtwaylandcompositor ${QT_MODULES}"
    ;;
  windows)
    export COMPILER_NAME="win64_msvc2022_64"
    export COMPILER_DIR="msvc2022_64"
    export AQT_PLATFORM="windows"
    export APP_ICON="$(cygpath -w "${PROJECT_ROOT}/resources/icons/app_icon.ico")"
    ;;
  darwin)
    export COMPILER_NAME="clang_64"
    export COMPILER_DIR="macos"
    export AQT_PLATFORM="mac"
    export APP_ICON="${PROJECT_ROOT}/resources/icons/app_icon.icns"
    ;;
  *)
    echo "Unsupported platform: ${PLATFORM}"
    exit 1
    ;;
esac

# --- DERIVED PATHS ---
export BUILD_DIR="${PROJECT_ROOT}/build"
export QT_INSTALL_DIR="${PROJECT_ROOT}/Qt"
export QT_ROOT="${QT_INSTALL_DIR}/${QT_VERSION}/${COMPILER_DIR}"
export QT_CMAKE_DIR="${QT_ROOT}/lib/cmake/Qt6"
export QT_BIN="${QT_ROOT}/bin"
export QT_PLUGINS_DIR="${QT_ROOT}/plugins"
export QT_PLATFORMS_DIR="${QT_PLUGINS_DIR}/platforms"
export QT_WAYLAND_DIR="${QT_PLUGINS_DIR}/wayland-shell-integration"
export CONAN_PROFILES_DIR="${PROJECT_ROOT}/conan/profiles"
export CONAN_PROFILE="${CONAN_PROFILES_DIR}/${PLATFORM}"
export CONAN_LOCK="${PROJECT_ROOT}/conan.lock"
export CONAN_TOOLCHAIN="${BUILD_DIR}/conan_toolchain.cmake"
export SRC_DIR="${PROJECT_ROOT}/src"
export FEATURES_DIR="${SRC_DIR}/features"

if [ "${PLATFORM}" == "windows" ]; then
    export BUILD_DIR="$(cygpath -w "${BUILD_DIR}")"
    export QT_INSTALL_DIR="$(cygpath -w "${QT_INSTALL_DIR}")"
    export QT_ROOT="$(cygpath -w "${QT_ROOT}")"
    export QT_CMAKE_DIR="$(cygpath -w "${QT_CMAKE_DIR}")"
    export QT_BIN="$(cygpath -w "${QT_BIN}")"
    export QT_PLUGINS_DIR="$(cygpath -w "${QT_PLUGINS_DIR}")"
    export QT_PLATFORMS_DIR="$(cygpath -w "${QT_PLATFORMS_DIR}")"
    export QT_WAYLAND_DIR="$(cygpath -w "${QT_WAYLAND_DIR}")"
    export CONAN_PROFILE="$(cygpath -w "${CONAN_PROFILE}")"
    export CONAN_TOOLCHAIN="$(cygpath -w "${CONAN_TOOLCHAIN}")"
fi