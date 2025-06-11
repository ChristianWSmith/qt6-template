#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

installPipenv

LOCKFILE_ARGS=""

for PROFILE in $(find "${CONAN_PROFILES_DIR}" -type f); do
  LOCK="$(mktemp --suffix=.lock)"
  LOCKFILE_ARGS="--lockfile=${LOCK} ${LOCKFILE_ARGS}"
  pipenv run conan lock create "${PROJECT_ROOT}" \
    --profile:build="${PROFILE}" \
    --profile:host="${PROFILE}" \
    --lockfile-out="${LOCK}"
done

pipenv run conan lock merge ${LOCKFILE_ARGS} --lockfile-out="${CONAN_LOCK}"
