#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

CMAKE_BUILD_TYPE="Debug"
BUILD_TESTING="OFF"
UPDATE_TRANSLATIONS="OFF"

EXTRA_ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
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

"${SCRIPT_DIR}/build.sh" \
  --build-type "${CMAKE_BUILD_TYPE}" \
  --test "${BUILD_TESTING}" \
  --update-translations "${UPDATE_TRANSLATIONS}"

if [ "${PLATFORM}" == "windows" ]; then
  "${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe" "${EXTRA_ARGS[@]}"
else
  "${BUILD_DIR}/${APP_NAME}" "${EXTRA_ARGS[@]}"
fi