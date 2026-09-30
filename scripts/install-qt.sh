#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

QT_MODULES="${QT_MODULES:-}"
MODULE_ARGS=()
if [[ -n "${QT_MODULES}" ]]; then
  MODULE_ARGS=(-m ${QT_MODULES})
fi

# Workaround for py7zr Windows path-check bug (py7zr#714 / aqtinstall#995):
# intermittent Bad7zFile "Specified path is bad" during aqt extraction.
# Use external 7-Zip on Windows to bypass py7zr.
AQT_EXTRA_ARGS=()
if [[ "${PLATFORM}" == "windows" ]]; then
  SEVENZIP=""
  for candidate in \
      "$(command -v 7z 2>/dev/null || true)" \
      "/c/Program Files/7-Zip/7z.exe" \
      "C:/Program Files/7-Zip/7z.exe"; do
    if [[ -n "${candidate}" && -f "${candidate}" ]]; then
      SEVENZIP="${candidate}"
      break
    fi
  done
  if [[ -z "${SEVENZIP}" ]]; then
    choco install 7zip -y
    SEVENZIP="C:/Program Files/7-Zip/7z.exe"
  fi
  if [[ -f "${SEVENZIP}" ]]; then
    AQT_EXTRA_ARGS=(--external "${SEVENZIP}")
  else
    echo "warning: 7z not found; aqt will use py7zr (may hit Bad7zFile on Windows)" >&2
  fi
fi

installPipenv

MAX_ATTEMPTS=3
for attempt in $(seq 1 "${MAX_ATTEMPTS}"); do
  echo "aqt install-qt attempt ${attempt}/${MAX_ATTEMPTS}"
  if pipenv run aqt install-qt \
      "${AQT_PLATFORM}" desktop "${QT_VERSION}" "${COMPILER_NAME}" \
      -O "${QT_INSTALL_DIR}" \
      "${AQT_EXTRA_ARGS[@]+"${AQT_EXTRA_ARGS[@]}"}" \
      "${MODULE_ARGS[@]+"${MODULE_ARGS[@]}"}"; then
    exit 0
  fi
  echo "aqt install-qt failed (attempt ${attempt}/${MAX_ATTEMPTS})" >&2
  if [[ "${attempt}" -lt "${MAX_ATTEMPTS}" ]]; then
    rm -rf "${PROJECT_ROOT}/Qt"
    sleep $((attempt * 5))
  fi
done

echo "error: aqt install-qt failed after ${MAX_ATTEMPTS} attempts" >&2
exit 1
