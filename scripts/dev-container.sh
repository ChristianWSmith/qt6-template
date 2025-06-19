#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

CONTAINER_NAME="$(echo "${APP_NAME}" | tr '[:upper:]' '[:lower:]' | tr -cd '[:alnum:]')-dev"

docker buildx build \
  -t "${CONTAINER_NAME}" \
  -f "${PROJECT_ROOT}/Dockerfile" \
  "${PROJECT_ROOT}"

GUI_ARGS="-e QT_QPA_PLATFORM=offscreen"
USING_XHOST="false"

RED="\033[31m"
YELLOW="\033[33m"
GREEN="\033[32m"
NEUTRAL="\033[0m"

case "$PLATFORM" in
  linux)
    if command -v xhost >/dev/null 2>&1; then
      echo -e "${GREEN}Setting up GUI forwarding for Linux...${NEUTRAL}"
      xhost +local:docker >/dev/null 2>&1;
      if [[ -z "${DISPLAY:-}" ]]; then
        echo -e "${YELLOW}Warning: DISPLAY is not set. GUI apps may not launch correctly.${NEUTRAL}"
      fi
      GUI_ARGS="-e DISPLAY=$DISPLAY -e QT_X11_NO_MITSHM=1 -v /tmp/.X11-unix:/tmp/.X11-unix -e QT_QPA_PLATFORM=xcb"
      USING_XHOST="true"
    else
      echo -e "${YELLOW}Warning: 'xhost' not found. GUI forwarding won't work on Linux without it.${NEUTRAL}"
    fi
    ;;

  darwin)
    echo -e "${GREEN}Setting up GUI forwarding for macOS...${NEUTRAL}"
    MAC_X11_HOST=${MAC_X11_HOST:-localhost}
    MAC_X11_PORT=${MAC_X11_PORT:-0}
    GUI_ARGS="-e DISPLAY=${MAC_X11_HOST}:${MAC_X11_PORT} -e QT_QPA_PLATFORM=xcb"
    echo -e "${YELLOW}Assuming XQuartz is installed and running. GUI forwarding won't work on macOS without it.${NEUTRAL}"
    echo -e "${YELLOW}Set MAC_X11_HOST and MAC_X11_PORT environment variables if you need something other than localhost:0${NEUTRAL}"
    ;;

  windows)
    echo -e "${GREEN}Setting up GUI forwarding for Windows...${NEUTRAL}"
    WIN_X11_HOST=${WIN_X11_HOST:-localhost}
    WIN_X11_PORT=${WIN_X11_PORT:-0}
    GUI_ARGS="-e DISPLAY=${WIN_X11_HOST}:${WIN_X11_PORT} -e QT_QPA_PLATFORM=xcb"
    echo -e "${YELLOW}Assuming VcXsrv or similar X server is installed and running. GUI forwarding won't work on Windows without it.${NEUTRAL}"
    echo -e "${YELLOW}Set WIN_X11_HOST and WIN_X11_PORT environment variables if you need something other than localhost:0${NEUTRAL}"
    ;;

  *)
    echo -e "${RED}Unsupported PLATFORM value: '$PLATFORM'. Falling back to offscreen rendering.${NEUTRAL}"
    GUI_ARGS="-e QT_QPA_PLATFORM=offscreen"
    ;;
esac

CONAN_HOME="${PROJECT_ROOT}/.conan2"
mkdir -p "${CONAN_HOME}"
LLDB_PORT="${LLDB_PORT:-12345}"

docker run \
  ${GUI_ARGS} \
  -e LANG="${LANG:-en_US.UTF-8}" \
  -e LC_ALL="${LC_ALL:-${LANG:-en_US.UTF-8}}" \
  -e WORKSPACE="${PROJECT_ROOT}" \
  --rm -it \
  -v "${CONAN_HOME}":"/home/devuser/.conan2" \
  -v "${PROJECT_ROOT}":"${PROJECT_ROOT}" \
  -w "${PROJECT_ROOT}" \
  "${CONTAINER_NAME}" bash

if [[ "${USING_XHOST}" == "true" ]]; then
  xhost -local:docker >/dev/null 2>&1;
fi
