#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

CMAKE_BUILD_TYPE="Release"
BUILD_TESTING="ON"
UPDATE_TRANSLATIONS="OFF"
CLEAN="OFF"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean)
      CLEAN="$2"
      shift 2
      ;;
    --build-type)
      CMAKE_BUILD_TYPE="$2"
      shift 2
      ;;
    --test)
      BUILD_TESTING="$2"
      shift 2
      ;;
    --update-translations)
      UPDATE_TRANSLATIONS="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

export CMAKE_BUILD_TYPE
export BUILD_TESTING
export UPDATE_TRANSLATIONS

if [ ! -e "${QT_ROOT}" ]; then
  ${SCRIPT_DIR}/install-qt.sh
fi

if [ "${CLEAN}" == "ON" ]; then 
    ${SCRIPT_DIR}/clean.sh build
fi

installPipenv

# Architecture boundary checks are part of the local build loop (not CI-only).
"${SCRIPT_DIR}/check-architecture-boundaries.sh"

# Source re-discovery relies on file(GLOB_RECURSE ... CONFIGURE_DEPENDS).
# If a generator ever misses a new file, touch CMakeLists.txt manually.

# Headless default must apply to gtest_discover_tests POST_BUILD discovery
# (runs inside the Conan/CMake build) as well as subsequent CTest execution.
if [ "${BUILD_TESTING}" == "ON" ]; then
  export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
fi

pipenv run conan build "${PROJECT_ROOT}" \
  --output-folder "${BUILD_DIR}" \
  --profile:build="${CONAN_PROFILE}" \
  --profile:host="${CONAN_PROFILE}" \
  -s build_type="${CMAKE_BUILD_TYPE}" \
  --build=missing \
  --lockfile="${CONAN_LOCK}"

if [ "${BUILD_TESTING}" == "ON" ]; then
  echo "Running tests (ctest --test-dir \"${BUILD_DIR}\")..."
  ctest --test-dir "${BUILD_DIR}" --output-on-failure

  # Production bootstrap smoke: validates cxxopts + QApplication + registerAll +
  # AppMainWindow + setTheme + translator setup without a display.
  if [ "${PLATFORM}" == "windows" ]; then
    APP_SMOKE_TARGET="${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe"
  elif [ "${PLATFORM}" == "darwin" ]; then
    APP_SMOKE_TARGET="${BUILD_DIR}/${APP_NAME}.app/Contents/MacOS/${APP_NAME}"
  else
    APP_SMOKE_TARGET="${BUILD_DIR}/${APP_NAME}"
  fi
  if [ -e "${APP_SMOKE_TARGET}" ]; then
    echo "Running production smoke-test (${APP_SMOKE_TARGET})..."
    QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}" "${APP_SMOKE_TARGET}" --smoke-test
  else
    echo "WARNING: smoke-test target not found: ${APP_SMOKE_TARGET}" >&2
  fi
else
  echo "Tests skipped (--test OFF). Build completed without running ctest."
fi
