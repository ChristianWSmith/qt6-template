#!/usr/bin/env bash
# Lightweight architectural boundary checks.
# Heuristic source-pattern checks only — not a C++ dependency graph, not a
# static-analysis framework, not a substitute for code review.
#
# EXEMPT (wiring / bootstrap / tests):
#   src/features/*/presenter/*   — may include model, widget, events
#   src/appmainwindow/**         — composition root fan-in
#   src/main.cpp                 — bootstrap
#   tests/**                     — white-box + fixture wiring
# ALLOWED (not a violation):
#   src/events/** -> logging/logging.h
#   src/services/** -> events/ + logging
#   src/platform/** -> core/ + logging
#   src/features/*/model/** -> core/ + logging
#
# Residual limitation: include-path and symbol greps can be evaded by
# unconventional include layouts or by avoiding banned identifiers; this is
# accepted for template scale.
set -euo pipefail

ROOT="${ARCH_CHECK_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
fail=0

# Report matches from a single file (grep -n output has line:content).
report_file_hits() {
  local file="$1" rule="$2" hits="$3"
  echo "VIOLATION (${rule}): ${file}"
  printf '%s\n' "${hits}" | sed "s|^|  ${file}:|"
}

# Scan one directory tree for #include lines whose path contains a forbidden
# segment (e.g. "model/" inside a widget file).
# check_layer_includes <scan_root> <scan_glob> <forbid_path> <rule>
check_layer_includes() {
  local scan_root="$1"
  local scan_glob="$2"
  local forbid_path="$3"
  local rule="$4"

  [ -d "${scan_root}" ] || return 0

  local f hits
  while IFS= read -r -d '' f; do
    if hits=$(grep -nE "#include[[:space:]]+[\"<][^\">]*${forbid_path}" "${f}" 2>/dev/null) \
      && [ -n "${hits}" ]; then
      report_file_hits "${f}" "${rule}" "${hits}"
      fail=1
    fi
  done < <(find "${scan_root}" -path "${scan_glob}" \
      \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) -print0 2>/dev/null)
}

# Basename ban for includes that omit a path segment (e.g. "CounterModel.h").
# Two-char prefix rule excludes IModel.h (single char before Model).
# check_basename_includes <scan_root> <scan_glob> <basename_regex> <rule>
check_basename_includes() {
  local scan_root="$1"
  local scan_glob="$2"
  local basename_regex="$3"
  local rule="$4"

  [ -d "${scan_root}" ] || return 0

  local f hits
  while IFS= read -r -d '' f; do
    if hits=$(grep -nE "#include[[:space:]]+[\"<][^\">]*${basename_regex}[\">]" "${f}" 2>/dev/null) \
      && [ -n "${hits}" ]; then
      report_file_hits "${f}" "${rule}" "${hits}"
      fail=1
    fi
  done < <(find "${scan_root}" -path "${scan_glob}" \
      \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) -print0 2>/dev/null)
}

MODEL_HDR_BASENAME='[A-Za-z0-9_][A-Za-z0-9_]+Model\.h'
WIDGET_HDR_BASENAME='[A-Za-z0-9_][A-Za-z0-9_]+Widget\.h'

# --- Rule 1: feature widgets must not include model headers ------------------
check_layer_includes "${ROOT}/src/features" '*/widget/*' 'model/' \
  'feature widget includes model/'
check_basename_includes "${ROOT}/src/features" '*/widget/*' \
  "${MODEL_HDR_BASENAME}" 'feature widget includes *Model.h'

# --- Rule 1w/1c: standalone widgets must not include model headers -----------
check_layer_includes "${ROOT}/src/widgets" '*' 'model/' \
  'standalone widget includes model/'
check_basename_includes "${ROOT}/src/widgets" '*' \
  "${MODEL_HDR_BASENAME}" 'standalone widget includes *Model.h'

# --- Rule 2: logging must not reference the EventSystem surface --------------
LOGGING_DIR="${ROOT}/src/logging"
if [ -d "${LOGGING_DIR}" ]; then
  LOGGING_ES_PATTERN='events::|LogEvent|EventSystem|BusRegistry|EventDispatcher|Subscription'
  if hits=$(grep -rnE "${LOGGING_ES_PATTERN}" "${LOGGING_DIR}" \
      --include='*.h' --include='*.hpp' --include='*.cpp' 2>/dev/null) \
    && [ -n "${hits}" ]; then
    echo "VIOLATION (logging references EventSystem surface)"
    printf '%s\n' "${hits}" | sed 's|^|  |'
    fail=1
  fi
fi

# --- Rule 3: feature models must not include widget headers ------------------
check_layer_includes "${ROOT}/src/features" '*/model/*' 'widget/' \
  'model includes widget/'
check_basename_includes "${ROOT}/src/features" '*/model/*' \
  "${WIDGET_HDR_BASENAME}" 'model includes *Widget.h'

# --- Rule 4: feature models must not include EventSystem headers -------------
check_layer_includes "${ROOT}/src/features" '*/model/*' 'EventSystem' \
  'model includes EventSystem'
check_layer_includes "${ROOT}/src/features" '*/model/*' 'events/' \
  'model includes events/'

# --- Rule 5: infrastructure must not depend on features ----------------------
for infra in events services platform; do
  check_layer_includes "${ROOT}/src/${infra}" '*' 'features/' \
    "${infra} includes features/"
done

# --- Rule 6: core must not depend on UI / events / features ------------------
check_layer_includes "${ROOT}/src/core" '*' 'widgets/' 'core includes widgets/'
check_layer_includes "${ROOT}/src/core" '*' 'events/' 'core includes events/'
check_layer_includes "${ROOT}/src/core" '*' 'features/' 'core includes features/'

# --- Result -----------------------------------------------------------------
if [ "${fail}" -ne 0 ]; then
  echo "Architecture boundary checks FAILED"
  exit 1
fi

echo "Architecture boundary checks passed"
