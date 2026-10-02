#!/usr/bin/env bash
# Shared platform-aware production binary path resolution (AUD-109 / AUD-003).
#
# Sourced, not executed. Requires env from scripts/env.sh:
#   PLATFORM, BUILD_DIR, APP_NAME, PROJECT_ROOT
# Optional:
#   CMAKE_BUILD_TYPE (default: Release) — used only for the Windows
#   multi-config fallback path.
#
# Exports:
#   APP_BIN         Absolute (or platform-native) path to the app executable.
#   APP_BIN_EXISTS  1 if APP_BIN exists, 0 otherwise. Consumers check this
#                   instead of re-implementing platform branches.
#   APP_BUNDLE      (Darwin only) path to the .app bundle directory.
#
# Resolution order:
#   Windows: try single-config ${BUILD_DIR}/${APP_NAME}.exe FIRST (Ninja,
#            the Conan profile default), fallback multi-config
#            ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe (VS).
#   Darwin:  ${BUILD_DIR}/${APP_NAME}.app/Contents/MacOS/${APP_NAME}
#   Linux:   ${BUILD_DIR}/${APP_NAME}
#
# Packaging scripts that need a DIST-specific copy path may keep their own
# layouts; they should still resolve the *built* binary via this helper.

: "${PLATFORM:?resolve-app-path.sh requires PLATFORM (source scripts/env.sh first)}"
: "${BUILD_DIR:?resolve-app-path.sh requires BUILD_DIR (source scripts/env.sh first)}"
: "${APP_NAME:?resolve-app-path.sh requires APP_NAME (source scripts/env.sh first)}"
: "${PROJECT_ROOT:?resolve-app-path.sh requires PROJECT_ROOT (source scripts/env.sh first)}"

CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"

case "${PLATFORM}" in
  windows)
    # Single-config Ninja (conan/profiles/windows pins generator=Ninja) places
    # the exe at the build root. Multi-config generators (Visual Studio) nest
    # under ${CMAKE_BUILD_TYPE}/. Prefer the Ninja layout — it is what this
    # repo pins — and fall back for VS-generated trees.
    _app_path_single="${BUILD_DIR}/${APP_NAME}.exe"
    _app_path_multi="${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe"
    if [ -e "${_app_path_single}" ]; then
      APP_BIN="${_app_path_single}"
    else
      APP_BIN="${_app_path_multi}"
    fi
    unset _app_path_single _app_path_multi
    ;;
  darwin)
    APP_BUNDLE="${BUILD_DIR}/${APP_NAME}.app"
    APP_BIN="${APP_BUNDLE}/Contents/MacOS/${APP_NAME}"
    ;;
  linux)
    APP_BIN="${BUILD_DIR}/${APP_NAME}"
    ;;
  *)
    echo "resolve-app-path.sh: unsupported PLATFORM='${PLATFORM}'" >&2
    return 1
    ;;
esac

export APP_BIN
if [ -e "${APP_BIN}" ]; then
  export APP_BIN_EXISTS=1
else
  export APP_BIN_EXISTS=0
fi
if [ -n "${APP_BUNDLE:-}" ]; then
  export APP_BUNDLE
fi
