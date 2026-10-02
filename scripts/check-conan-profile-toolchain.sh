#!/usr/bin/env bash
# Compare conan/profiles/* compiler.version pins against the detected host compiler.
# Exit 1 on DRIFT. Exit 0 on MATCH or when detection is unavailable (UNKNOWN).
# Wired into CI after "Toolchain versions"; also useful locally before release builds.
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJECT_ROOT=$(realpath "${SCRIPT_DIR}/..")
PROFILES_DIR="${PROJECT_ROOT}/conan/profiles"

profile_get() { # $1=file $2=key
  sed -n "s/^${2}=//p" "$1" | head -1
}

detect_gcc_major() {
  local cc="${CXX:-g++}"
  command -v "$cc" >/dev/null 2>&1 || { echo ""; return; }
  "$cc" -dumpversion 2>/dev/null | cut -d. -f1
}

detect_apple_clang_major() {
  local cxx="${CXX:-clang++}"
  command -v "$cxx" >/dev/null 2>&1 || { echo ""; return; }
  "$cxx" --version 2>/dev/null \
    | sed -n 's/.*Apple clang version \([0-9][0-9]*\).*/\1/p' | head -1
}

detect_msvc_conan_version() {
  command -v cl >/dev/null 2>&1 || { echo ""; return; }
  local msc
  msc=$(cl 2>&1 | sed -n 's/.*[^0-9]\([0-9][0-9][0-9][0-9]\)[^0-9].*/\1/p' | head -1)
  [ -n "${msc}" ] || { echo ""; return; }
  # _MSC_VER 19xx -> Conan msvc version 19x (1940-1949->194, 1950-1959->195)
  echo $(( msc / 10 ))
}

fail=0
report() { # $1 label $2 pin $3 detected $4 human-name
  if [ -z "${2}" ]; then
    echo "[$1] pin=(missing)  detected=${3:-?}  -> FAIL (profile has no compiler.version)"
    fail=1; return
  fi
  if [ -z "${3}" ]; then
    echo "[$1] pin=${2}  detected=(unavailable)  -> UNKNOWN (not failing; probe missing)"
    return
  fi
  if [ "${2}" = "${3}" ]; then
    echo "[$1] pin=${2}  detected=${3}  -> MATCH ($4)"
  else
    echo "[$1] pin=${2}  detected=${3}  -> DRIFT ($4)"
    fail=1
  fi
}

echo "=== Conan profile pin vs detected toolchain ==="
echo "Host: $(uname -s)/$(uname -m)  CXX=${CXX:-<default>}"

case "$(uname -s)" in
  Linux)
    report "linux" \
      "$(profile_get "${PROFILES_DIR}/linux" compiler.version)" \
      "$(detect_gcc_major)" \
      "gcc major"
    echo "[linux] note: pin may target an incoming ubuntu-26.04 runner (gcc 15)."
    ;;
  Darwin)
    report "darwin" \
      "$(profile_get "${PROFILES_DIR}/darwin" compiler.version)" \
      "$(detect_apple_clang_major)" \
      "apple-clang major"
    arch_pin="$(profile_get "${PROFILES_DIR}/darwin" arch)"
    arch_det="$(uname -m)"
    # Normalize arm64/armv8
    if [ "${arch_det}" = "arm64" ]; then arch_det="armv8"; fi
    if [ -n "${arch_pin}" ] && [ "${arch_pin}" != "${arch_det}" ]; then
      echo "[darwin] pin arch=${arch_pin}  host=${arch_det}  -> DRIFT (arch)"
      fail=1
    else
      echo "[darwin] arch pin=${arch_pin} host=${arch_det} -> MATCH"
    fi
    ;;
  MINGW*|MSYS*|CYGWIN*)
    report "windows" \
      "$(profile_get "${PROFILES_DIR}/windows" compiler.version)" \
      "$(detect_msvc_conan_version)" \
      "msvc Conan version from _MSC_VER"
    vs="$(sed -n 's/^tools\.microsoft\.msbuild:vs_version=//p' "${PROFILES_DIR}/windows" | head -1)"
    echo "[windows] conf tools.microsoft.msbuild:vs_version=${vs:-<unset>} (expected 18 for VS 2026 runners)"
    echo "[windows] note: msvc 194 is a deliberate v143 compat pin on VS 2026 images."
    ;;
  *)
    echo "Unsupported host $(uname -s); skipping pin check."
    ;;
esac

if [ "${fail}" -ne 0 ]; then
  echo ""
  echo "RESULT: DRIFT. Update conan/profiles/<platform> or pin runs-on deliberately."
  echo "Reminder: a pin matching an *incoming* runner image may be intentional —"
  echo "annotate the profile so the next reader does not 'fix' it backward."
  exit 1
fi
echo ""
echo "RESULT: pins match detected toolchains."
