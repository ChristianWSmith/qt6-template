#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

installPipenv

# Safety contract (F-20):
#   create temporary locks
#   perform merge/update
#   success? → atomic mv/replace over conan.lock
#   failure? → preserve existing lock
#   always clean temp artifacts
TMP_FILES=()
cleanup() {
  rm -f "${TMP_FILES[@]+"${TMP_FILES[@]}"}"
}
trap cleanup EXIT

LOCK_ARGS=()

for PROFILE in $(find "${CONAN_PROFILES_DIR}" -type f); do
  # Portable mktemp (no GNU --suffix); works on macOS/BSD and Linux.
  LOCK="$(mktemp "${TMPDIR:-/tmp}/qt6-template-lock.XXXXXX")"
  TMP_FILES+=("${LOCK}")
  LOCK_ARGS+=("--lockfile=${LOCK}")
  pipenv run conan lock create "${PROJECT_ROOT}" \
    --profile:build="${PROFILE}" \
    --profile:host="${PROFILE}" \
    --lockfile-out="${LOCK}"
done

# Merge into a temp file first; only replace conan.lock after success.
# On failure the existing conan.lock is preserved (never deleted upfront).
MERGED_LOCK="$(mktemp "${TMPDIR:-/tmp}/qt6-template-lock-merged.XXXXXX")"
TMP_FILES+=("${MERGED_LOCK}")

pipenv run conan lock merge "${LOCK_ARGS[@]}" --lockfile-out="${MERGED_LOCK}"

# Replace conan.lock only after a successful merge.
mv -f "${MERGED_LOCK}" "${CONAN_LOCK}"
