#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

QT_MODULES="${QT_MODULES:-}"
MODULE_ARGS=""
if [[ -n "{$QT_MODULES}" ]]; then
  MODULE_ARGS="-m ${QT_MODULES}"
fi

pipenv install --dev
pipenv run aqt install-qt "${AQT_PLATFORM}" desktop "${QT_VERSION}" "${COMPILER_NAME}" -O "${QT_INSTALL_DIR}" ${MODULE_ARGS}
