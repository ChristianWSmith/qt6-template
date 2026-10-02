#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/../../scripts/env.sh
# Built-binary resolution (AUD-003/AUD-109). Keep APP_BIN as the AppDir
# destination below — the helper result is saved under a distinct name.
source "${SCRIPT_DIR}/../../scripts/lib/resolve-app-path.sh"
BUILD_APP_BIN="${APP_BIN}"

mkdir -p "${DIST_DIR}"

export APP_DIR="${PROJECT_ROOT}/${APP_NAME}.AppDir"
export BIN_DIR="${APP_DIR}/usr/bin"
export LIB_DIR="${APP_DIR}/usr/lib"
export APP_BIN="${BIN_DIR}/${APP_NAME}"

export APP_PLUGINS_DIR="${APP_DIR}/usr/plugins"

# AUD-111: pin appimagetool to a released tag + sha256 (no floating
# "continuous" channel). Tag 1.9.1 is the latest non-continuous release.
APPIMAGETOOL_VERSION="1.9.1"
APPIMAGETOOL_URL="https://github.com/AppImage/appimagetool/releases/download/${APPIMAGETOOL_VERSION}/appimagetool-x86_64.AppImage"
APPIMAGETOOL_SHA256="ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0"
APPIMAGETOOL_PATH="${PROJECT_ROOT}/appimagetool"

fetch_appimagetool() {
    curl -fsSL "${APPIMAGETOOL_URL}" -o "${APPIMAGETOOL_PATH}"
}

# Verify checksum on every run; re-download when missing or mismatched.
if ! echo "${APPIMAGETOOL_SHA256}  ${APPIMAGETOOL_PATH}" 2>/dev/null | sha256sum -c - >/dev/null 2>&1; then
    echo "Fetching appimagetool ${APPIMAGETOOL_VERSION} (${APPIMAGETOOL_URL})..."
    fetch_appimagetool
fi
if ! echo "${APPIMAGETOOL_SHA256}  ${APPIMAGETOOL_PATH}" | sha256sum -c -; then
    echo "error: appimagetool sha256 mismatch (expected ${APPIMAGETOOL_SHA256})" >&2
    rm -f "${APPIMAGETOOL_PATH}"
    exit 1
fi
chmod +x "${APPIMAGETOOL_PATH}"

mkdir -p "${BIN_DIR}"
mkdir -p "${LIB_DIR}"
mkdir -p "${APP_PLUGINS_DIR}"
cp -r "${QT_PLUGINS_DIR}/." "${APP_PLUGINS_DIR}/"
cp "${BUILD_APP_BIN}" "${APP_BIN}"
chmod +x "${APP_BIN}"

seen_deps=()
deps=()

gather_deps() {
    local bin="$1"
    while IFS= read -r dep; do
        [[ -z "${dep}" ]] && continue

        if [[ ! -e "${dep}" ]]; then
            exit 1
        fi

        for existing in "${seen_deps[@]}"; do
            [[ "${existing}" == "${dep}" ]] && continue 2
        done

        seen_deps+=("${dep}")
        deps+=("${dep}")
        gather_deps "${dep}"
    done < <(ldd "${bin}" | awk '/=>/ {print $3}' | grep -v '^(')
}

gather_deps "${APP_BIN}"

# AUD-111: vendored excludelist — do not wget pkg2appimage master.
EXCLUDELIST_PATH="${SCRIPT_DIR}/appimage-excludelist"
if [ ! -f "${EXCLUDELIST_PATH}" ]; then
    echo "error: vendored excludelist missing: ${EXCLUDELIST_PATH}" >&2
    exit 1
fi
BLACKLIST_REGEX=$(grep -vE '^\s*#|^\s*$' "${EXCLUDELIST_PATH}" | \
    cut -d'#' -f1 | \
    sed -E 's/^[[:space:]]+|[[:space:]]+$//g' | \
    sed -E 's/[][\.^$]/\\&/g' | \
    sed 's/^/^/' | \
    sed 's/$/$/' | \
    paste -sd'|' -)

for dep in "${deps[@]}"; do
    real_dep=$(readlink -f "${dep}")
    base=$(basename "${real_dep}")
    if [[ ! "${base}" =~ ${BLACKLIST_REGEX} ]]; then
    cp -v "${dep}" "${LIB_DIR}/" || true
    else
    echo "Skipping blacklisted dependency: ${base}"
    fi
done

patchelf --set-rpath '$ORIGIN/../lib' "${APP_BIN}"

cat > "${APP_DIR}/AppRun" <<EOF
#!/bin/sh
export QT_PLUGIN_PATH="\${APPDIR}/usr/plugins"
"\${APPDIR}/usr/bin/${APP_NAME}" "\$@"
EOF
chmod +x "${APP_DIR}/AppRun"

cat > "${APP_DIR}/${APP_ID}.desktop" <<EOF
[Desktop Entry]
Type=Application
X-AppVersion=${APP_VERSION}
Name=${APP_NAME}
Exec=${APP_NAME}
Icon=${APP_ID}
Categories=${APP_CATEGORIES}
StartupWMClass=${APP_NAME}
EOF

IMAGEMAGICK_CMD="magick"
if ! command -v magick &> /dev/null; then
    IMAGEMAGICK_CMD="convert"
fi

"${IMAGEMAGICK_CMD}" "${APP_ICON}" "${APP_DIR}/${APP_ID}.png"

rm -rf "${APP_NAME}-x86_64.AppImage"
rm -rf "${APP_NAME}.AppImage"
"${APPIMAGETOOL_PATH}" "${APP_DIR}"
rm -rf "${APP_DIR}"
mv "${PROJECT_ROOT}/${APP_NAME}-x86_64.AppImage" "${DIST_DIR}/${APP_NAME}-${APP_VERSION}.AppImage"

"${DIST_DIR}/${APP_NAME}-${APP_VERSION}.AppImage" --smoke-test
