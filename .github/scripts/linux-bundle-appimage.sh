#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/../../scripts/env.sh

export APP_DIR="${PROJECT_ROOT}/${APP_NAME}.AppDir"
export BIN_DIR="${APP_DIR}/usr/bin"
export LIB_DIR="${APP_DIR}/usr/lib"
export APP_BIN="${BIN_DIR}/${APP_NAME}"

export APP_PLUGINS_DIR="${APP_DIR}/usr/plugins"

echo "Installing appimagetool..."
wget https://github.com/AppImage/AppImageKit/releases/latest/download/appimagetool-x86_64.AppImage -O appimagetool
chmod +x appimagetool

echo "Setting up AppDir..."
mkdir -p "${BIN_DIR}"
mkdir -p "${LIB_DIR}"
mkdir -p "${APP_PLUGINS_DIR}"
cp -r "${QT_PLUGINS_DIR}/." "${APP_PLUGINS_DIR}/"
cp "${BUILD_DIR}/${APP_NAME}" "${APP_BIN}"
chmod +x "${APP_BIN}"

seen_deps=()
deps=()

gather_deps() {
    local bin="$1"
    while IFS= read -r dep; do
    [[ -z "${dep}" ]] && continue

    if [[ ! -e "${dep}" ]]; then
        echo "ERROR: Dependency not found or broken symlink: ${dep}" >&2
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

echo "Gathering dependencies..."
gather_deps "${APP_BIN}"

echo "Getting dependency blacklist..."
EXCLUDELIST_PATH="$(mktemp)"
wget -qO "${EXCLUDELIST_PATH}" https://raw.githubusercontent.com/AppImage/pkg2appimage/master/excludelist
BLACKLIST_REGEX=$(grep -vE '^\s*#|^\s*$' "${EXCLUDELIST_PATH}" | \
    cut -d'#' -f1 | \
    sed -E 's/^[[:space:]]+|[[:space:]]+$//g' | \
    sed -E 's/[][\.^$]/\\&/g' | \
    sed 's/^/^/' | \
    sed 's/$/$/' | \
    paste -sd'|' -)

echo "Copying dependencies..."
for dep in "${deps[@]}"; do
    real_dep=$(readlink -f "${dep}")
    base=$(basename "${real_dep}")
    if [[ ! "${base}" =~ ${BLACKLIST_REGEX} ]]; then
    cp -v "${dep}" "${LIB_DIR}/" || true
    else
    echo "Skipping blacklisted dependency: ${base}"
    fi
done

echo "Patching RPATH..."
patchelf --set-rpath '$ORIGIN/../lib' "${APP_BIN}"

echo "Finalizing AppDir..."
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

convert "${APP_ICON}" "${APP_DIR}/${APP_ID}.png"

echo "Building AppImage..."
rm -rf "${APP_NAME}-x86_64.AppImage"
rm -rf "${APP_NAME}.AppImage"
"${PROJECT_ROOT}/appimagetool" "${APP_DIR}"
rm -rf "${APP_DIR}"
mv "${PROJECT_ROOT}/${APP_NAME}-x86_64.AppImage" "${PROJECT_ROOT}/${APP_NAME}-${APP_VERSION}.AppImage"
