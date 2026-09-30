#!/usr/bin/env bash
# Lightweight architectural boundary checks (Wave 5 / F-17).
# Source-pattern checks only — no compilation, no static-analysis framework.
#
# Rules:
#   1. Feature widgets must not include model headers.
#   2. Production logging must not reference EventSystem / LogEvent / publish.
#   3. Feature models must not include widget headers.
#
# Presenters and the composition root may include both model and widget —
# they are wiring layers, not checked here.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fail=0

# Report matches from a single file (grep -n output has line:content).
report_file_hits() {
  local file="$1" rule="$2" hits="$3"
  echo "VIOLATION (${rule}): ${file}"
  printf '%s\n' "${hits}" | sed "s|^|  ${file}:|"
}

# Scan one directory tree for #include lines whose path contains a forbidden
# segment (e.g. "model/" inside a widget file).
check_layer_includes() {
  local scan_glob="$1"   # find -path pattern relative to src/features, e.g. */widget/*
  local forbid_path="$2" # path segment that must not appear in includes
  local rule="$3"

  local f hits
  while IFS= read -r -d '' f; do
    if hits=$(grep -nE "#include[[:space:]]+[\"<][^\">]*${forbid_path}" "${f}" 2>/dev/null) \
      && [ -n "${hits}" ]; then
      report_file_hits "${f}" "${rule}" "${hits}"
      fail=1
    fi
  done < <(find "${ROOT}/src/features" -path "${scan_glob}" \
      \( -name '*.h' -o -name '*.cpp' \) -print0 2>/dev/null)
}

# --- Rule 1: widgets must not include model headers -------------------------
check_layer_includes '*/widget/*' 'model/' 'widget includes model'

# --- Rule 2: logging must not reference EventSystem / LogEvent / publish ----
LOGGING_DIR="${ROOT}/src/logging"
if [ -d "${LOGGING_DIR}" ]; then
  if hits=$(grep -rnE 'events::publish|LogEvent|EventSystem' "${LOGGING_DIR}" \
      --include='*.h' --include='*.hpp' --include='*.cpp' 2>/dev/null) \
    && [ -n "${hits}" ]; then
    echo "VIOLATION (logging references EventSystem/LogEvent/publish)"
    printf '%s\n' "${hits}" | sed 's|^|  |'
    fail=1
  fi
fi

# --- Rule 3: models must not include widget headers -------------------------
check_layer_includes '*/model/*' 'widget/' 'model includes widget'

# --- Result -----------------------------------------------------------------
if [ "${fail}" -ne 0 ]; then
  echo "Architecture boundary checks FAILED"
  exit 1
fi

echo "Architecture boundary checks passed"
