#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh
pipenv install --dev
${SCRIPT_DIR}/install_qt.sh
${SCRIPT_DIR}/build.sh