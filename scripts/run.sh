#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

CMAKE_BUILD_TYPE="Debug"
BUILD_TESTING="OFF"
UPDATE_TRANSLATIONS="OFF"
REBUILD="OFF"
CLEAN="OFF"

EXTRA_ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean)
      CLEAN="$2"
      shift 2
      ;;
    --rebuild)
      REBUILD="$2"
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
    --)
      shift
      EXTRA_ARGS+=("$@")
      break
      ;;
    *)
      EXTRA_ARGS+=("$1")
      shift
      ;;
  esac
done

export CMAKE_BUILD_TYPE
export BUILD_TESTING
export UPDATE_TRANSLATIONS

if [ "${PLATFORM}" == "windows" ]; then
  APP_FINAL_TARGET="${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe"
elif [ "${PLATFORM}" == "darwin" ]; then
  APP_FINAL_TARGET="${BUILD_DIR}/${APP_NAME}.app"
else
  APP_FINAL_TARGET="${BUILD_DIR}/${APP_NAME}"
fi

if [ "${CLEAN}" == "ON" ]; then 
    ${SCRIPT_DIR}/clean.sh build
fi

if [ "${REBUILD}" == "ON" ] || [ ! -e "${APP_FINAL_TARGET}" ]; then
  "${SCRIPT_DIR}/build.sh" \
    --build-type "${CMAKE_BUILD_TYPE}" \
    --test "${BUILD_TESTING}" \
    --update-translations "${UPDATE_TRANSLATIONS}"
fi

if [ "${PLATFORM}" == "darwin" ]; then
  open -n "${APP_FINAL_TARGET}" --args "${EXTRA_ARGS[@]}"
else
  "${APP_FINAL_TARGET}" "${EXTRA_ARGS[@]}"
fi