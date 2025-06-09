#!/usr/bin/env bash
set -euo pipefail

source ./env.sh

# Optional: Add modules conditionally
QT_MODULES="${QT_MODULES:-}"
MODULE_ARGS=""
if [[ -n "$QT_MODULES" ]]; then
  MODULE_ARGS="-m $QT_MODULES"
fi

pipenv run aqt install-qt "$AQT_PLATFORM" desktop "$QT_VERSION" "$COMPILER_NAME" -O "Qt" $MODULE_ARGS
