#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

export CMAKE_BUILD_TYPE="${1:-Release}"

if [ ! -e "${QT_ROOT}" ]; then
  ${SCRIPT_DIR}/install_qt.sh
fi

pipenv install --dev
pipenv run conan build "${PROJECT_ROOT}" \
  --output-folder "${BUILD_DIR}" \
  --profile:build="${CONAN_PROFILE}" \
  --profile:host="${CONAN_PROFILE}" \
  -s build_type="${CMAKE_BUILD_TYPE}" \
  --build=missing \
  --lockfile="${CONAN_LOCK}"
