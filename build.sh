#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

export CMAKE_CONFIG="${1:-Release}"

# pipenv run conan install . \
#   --output-folder="${BUILD_DIR}" \
#   --build=missing \
#   --profile:build="${CONAN_PROFILE}" \
#   --profile:host="${CONAN_PROFILE}"
  
# cmake -B "${BUILD_DIR}" \
#   -DCMAKE_TOOLCHAIN_FILE="${CONAN_TOOLCHAIN}" \
#   -DCMAKE_PREFIX_PATH="${QT_CMAKE_DIR}" \
#   -DCMAKE_BUILD_TYPE=${CMAKE_CONFIG} \
#   -DQT_DEBUG_FIND_PACKAGE=ON

# cmake --build "${BUILD_DIR}" --parallel --config ${CMAKE_CONFIG}

# if [[ "${PLATFORM}" == "windows" ]]; then
#   cmd.exe /c "cd \"${BUILD_DIR}\" && python -m pipenv run conan build \"${PROJECT_ROOT}\" -of \"${BUILD_DIR}\""
# else
  # pushd "${BUILD_DIR}"
  pipenv run conan build "${PROJECT_ROOT}" \
    --output-folder "${BUILD_DIR}" \
    --profile:build="${CONAN_PROFILE}" \
    --profile:host="${CONAN_PROFILE}" \
    --build=missing
  # popd
# fi
