#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

CMAKE_BUILD_TYPE="Release"
BUILD_TESTING="ON"
UPDATE_TRANSLATIONS="ON"

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
  ${SCRIPT_DIR}/install_qt.sh
fi

installPipenv

pipenv run conan build "${PROJECT_ROOT}" \
  --output-folder "${BUILD_DIR}" \
  --profile:build="${CONAN_PROFILE}" \
  --profile:host="${CONAN_PROFILE}" \
  -s build_type="${CMAKE_BUILD_TYPE}" \
  --build=missing \
  --lockfile="${CONAN_LOCK}"
