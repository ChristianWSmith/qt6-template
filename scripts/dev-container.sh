#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

CONTAINER_NAME="$(echo "${APP_NAME}" | tr '[:upper:]' '[:lower:]' | tr -cd '[:alnum:]')-dev"

docker buildx build -t "${CONTAINER_NAME}" -f "${PROJECT_ROOT}/Dockerfile" "${PROJECT_ROOT}"
docker run --rm -it -v "${PROJECT_ROOT}":/workspace -w /workspace "${CONTAINER_NAME}" bash
