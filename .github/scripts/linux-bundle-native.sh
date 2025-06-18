
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/../../scripts/env.sh

mkdir -p "${DIST_DIR}"

ARCHIVE_NAME="${APP_NAME}-${APP_VERSION}"

SOURCE_BINARY_PATH="${BUILD_DIR}/${APP_NAME}"

SOURCE_ICONS_DIR="${ICONS_DIR}"

SOURCE_LICENSE_PATH="${PROJECT_ROOT}/LICENSE"

SOURCE_README_PATH="${PROJECT_ROOT}/README.md"

DESKTOP_TEMPLATE_NAME="${APP_ID}.desktop.in"
DESKTOP_ENTRY_NAME="${APP_ID}.desktop"
DESKTOP_ENTRY_CATEGORY="${APP_CATEGORIES}"
DESKTOP_ENTRY_COMMENT="${APP_DESCRIPTION}"

MAKEFILE_PREFIX="/usr/local"
MAKEFILE_USER_PREFIX="\$(HOME)/.local"


TARGET_ROOT_DIR="${PROJECT_ROOT}/${ARCHIVE_NAME}"
mkdir -p "${TARGET_ROOT_DIR}"

mkdir -p "${TARGET_ROOT_DIR}/bin"
mkdir -p "${TARGET_ROOT_DIR}/share/applications"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/scalable/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/16x16/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/24x24/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/32x32/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/48x48/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/64x64/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/128x128/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/256x256/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/512x512/apps"
echo "Created subdirectories."

cp "${SOURCE_BINARY_PATH}" "${TARGET_ROOT_DIR}/bin/${APP_NAME}"
chmod +x "${TARGET_ROOT_DIR}/bin/${APP_NAME}"
patchelf --set-rpath '/usr/lib' "${TARGET_ROOT_DIR}/bin/${APP_NAME}"

cat << EOF > "${TARGET_ROOT_DIR}/share/applications/${DESKTOP_TEMPLATE_NAME}"
[Desktop Entry]
Type=Application
X-AppVersion=${APP_VERSION}
Name=${APP_NAME}
Exec=@APP_EXEC_PATH@
Icon=${APP_ID}
Categories=${APP_CATEGORIES}
StartupWMClass=${APP_NAME}
EOF
echo "Created desktop template file: ${DESKTOP_TEMPLATE_NAME}"

echo "Copying and resizing icons from ${SOURCE_ICONS_DIR}..."

if ! command -v magick &> /dev/null && ! command -v convert &> /dev/null
then
    echo "Error: ImageMagick 'magick' or 'convert' command not found. Cannot resize PNG icons."
    echo "Please install ImageMagick (e.g., 'sudo apt install imagemagick' or 'sudo dnf install ImageMagick')."
    exit 1
fi

IMAGEMAGICK_CMD="magick"
if ! command -v magick &> /dev/null; then
    IMAGEMAGICK_CMD="convert"
fi

PNG_SIZES=(16x16 24x24 32x32 48x48 64x64 128x128 256x256 512x512)

SOURCE_SVG_ICON="${SOURCE_ICONS_DIR}/app_icon.svg"
SOURCE_PNG_1024_ICON="${SOURCE_ICONS_DIR}/app_icon.png"

cp "${SOURCE_SVG_ICON}" "${TARGET_ROOT_DIR}/share/icons/hicolor/scalable/apps/${APP_ID}.svg"

for size in "${PNG_SIZES[@]}"; do
    ICON_DIR="${TARGET_ROOT_DIR}/share/icons/hicolor/${size}/apps"
    mkdir -p "${ICON_DIR}"
    "${IMAGEMAGICK_CMD}" "${SOURCE_PNG_1024_ICON}" -resize "${size}" "${ICON_DIR}/${APP_ID}.png"
done

cp "${SOURCE_LICENSE_PATH}" "${TARGET_ROOT_DIR}/LICENSE"
cp "${SOURCE_README_PATH}" "${TARGET_ROOT_DIR}/README.md"

cat << EOF > "${TARGET_ROOT_DIR}/Makefile"

PREFIX ?= ${MAKEFILE_PREFIX}

USER_PREFIX := \$(HOME)/.local

BIN_DIR := \$(PREFIX)/bin
SHARE_APP_DIR := \$(PREFIX)/share/applications
SHARE_ICON_DIR := \$(PREFIX)/share/icons/hicolor

USER_BIN_DIR := \$(USER_PREFIX)/bin
USER_SHARE_APP_DIR := \$(USER_PREFIX)/share/applications
USER_SHARE_ICON_DIR := \$(USER_PREFIX)/share/icons/hicolor

APP_EXECUTABLE := bin/${APP_NAME}

DESKTOP_TEMPLATE := share/applications/${DESKTOP_TEMPLATE_NAME}
DESKTOP_FILE_FINAL := ${DESKTOP_ENTRY_NAME}

APP_ICON_NAME := ${APP_ID}
PNG_SIZES := 16x16 24x24 32x32 48x48 64x64 128x128 256x256 512x512

.PHONY: all install uninstall install-user uninstall-user clean

all:

install:
	@echo "Installing \$(APP_EXECUTABLE) to \$(BIN_DIR)..."
	mkdir -p "\$(BIN_DIR)"
	cp "\$(APP_EXECUTABLE)" "\$(BIN_DIR)/"
	chmod +x "\$(BIN_DIR)/${APP_NAME}"

	@echo "Creating and installing desktop file to \$(SHARE_APP_DIR)..."
	mkdir -p "\$(SHARE_APP_DIR)"
	
	sed "s|@APP_EXEC_PATH@|\$(BIN_DIR)/${APP_NAME}|g" "\$(DESKTOP_TEMPLATE)" > "\$(SHARE_APP_DIR)/\$(DESKTOP_FILE_FINAL)"

	@echo "Installing icons to \$(SHARE_ICON_DIR)..."
	
	mkdir -p "\$(SHARE_ICON_DIR)/scalable/apps"
	cp "share/icons/hicolor/scalable/apps/\$(APP_ICON_NAME).svg" "\$(SHARE_ICON_DIR)/scalable/apps/"
	
	\$(foreach size,\$(PNG_SIZES),\
		mkdir -p "\$(SHARE_ICON_DIR)/\$(size)/apps" && \
		cp "share/icons/hicolor/\$(size)/apps/\$(APP_ICON_NAME).png" "\$(SHARE_ICON_DIR)/\$(size)/apps/" ;\
	)
	@echo "Updating desktop database..."
	update-desktop-database "\$(SHARE_APP_DIR)" || true

	@echo "Updating icon cache..."
	gtk-update-icon-cache -qtf "\$(SHARE_ICON_DIR)" || true
	@echo "Installation complete. You may need to log out and back in for icons/desktop entries to appear."

uninstall:
	@echo "Uninstalling \$(APP_EXECUTABLE) from \$(BIN_DIR)..."
	rm -f "\$(BIN_DIR)/${APP_NAME}"

	@echo "Uninstalling desktop file from \$(SHARE_APP_DIR)..."
	rm -f "\$(SHARE_APP_DIR)/\$(DESKTOP_FILE_FINAL)"

	@echo "Uninstalling icons from \$(SHARE_ICON_DIR)..."
	rm -f "\$(SHARE_ICON_DIR)/scalable/apps/\$(APP_ICON_NAME).svg"
	\$(foreach size,\$(PNG_SIZES),\
		rm -f "\$(SHARE_ICON_DIR)/\$(size)/apps/\$(APP_ICON_NAME).png" ;\
	)
	@echo "Updating desktop database..."
	update-desktop-database "\$(SHARE_APP_DIR)" || true
	@echo "Updating icon cache..."
	gtk-update-icon-cache -qtf "\$(SHARE_ICON_DIR)" || true
	@echo "Uninstallation complete."

install-user:
	@echo "Installing \$(APP_EXECUTABLE) to \$(USER_BIN_DIR)..."
	mkdir -p "\$(USER_BIN_DIR)"
	cp "\$(APP_EXECUTABLE)" "\$(USER_BIN_DIR)/"
	chmod +x "\$(USER_BIN_DIR)/${APP_NAME}"

	@echo "Creating and installing desktop file to \$(USER_SHARE_APP_DIR)..."
	mkdir -p "\$(USER_SHARE_APP_DIR)"
	
	sed "s|@APP_EXEC_PATH@|\$(USER_BIN_DIR)/${APP_NAME}|g" "\$(DESKTOP_TEMPLATE)" > "\$(USER_SHARE_APP_DIR)/\$(DESKTOP_FILE_FINAL)"

	@echo "Installing icons to \$(USER_SHARE_ICON_DIR)..."
	
	mkdir -p "\$(USER_SHARE_ICON_DIR)/scalable/apps"
	cp "share/icons/hicolor/scalable/apps/\$(APP_ICON_NAME).svg" "\$(USER_SHARE_ICON_DIR)/scalable/apps/"
	
	\$(foreach size,\$(PNG_SIZES),\
		mkdir -p "\$(USER_SHARE_ICON_DIR)/\$(size)/apps" && \
		cp "share/icons/hicolor/\$(size)/apps/\$(APP_ICON_NAME).png" "\$(USER_SHARE_ICON_DIR)/\$(size)/apps/" ;\
	)
	@echo "Updating desktop database..."
	update-desktop-database "\$(USER_SHARE_APP_DIR)" || true
	@echo "Updating icon cache..."
	gtk-update-icon-cache -qtf "\$(USER_SHARE_ICON_DIR)" || true
	@echo "User installation complete. You may need to log out and back in for icons/desktop entries to appear."

uninstall-user:
	@echo "Uninstalling \$(APP_EXECUTABLE) from \$(USER_BIN_DIR)..."
	rm -f "\$(USER_BIN_DIR)/${APP_NAME}"

	@echo "Uninstalling desktop file from \$(USER_SHARE_APP_DIR)..."
	rm -f "\$(USER_SHARE_APP_DIR)/\$(DESKTOP_FILE_FINAL)"

	@echo "Uninstalling icons from \$(USER_SHARE_ICON_DIR)..."
	rm -f "\$(USER_SHARE_ICON_DIR)/scalable/apps/\$(APP_ICON_NAME).svg"
	\$(foreach size,\$(PNG_SIZES),\
		rm -f "\$(USER_SHARE_ICON_DIR)/\$(size)/apps/\$(APP_ICON_NAME).png" ;\
	)
	@echo "Updating desktop database..."
	update-desktop-database "\$(USER_SHARE_APP_DIR)" || true
	@echo "Updating icon cache..."
	gtk-update-icon-cache -qtf "\$(USER_SHARE_ICON_DIR)" || true
	@echo "User uninstallation complete."

clean:
	
EOF
echo "Created Makefile."

tar -czvf "${ARCHIVE_NAME}.tar.gz" -C "$(dirname "${TARGET_ROOT_DIR}")" "$(basename "${TARGET_ROOT_DIR}")"
mv "${PROJECT_ROOT}/${ARCHIVE_NAME}.tar.gz" "${DIST_DIR}/${ARCHIVE_NAME}.tar.gz"
rm -rf "${TARGET_ROOT_DIR}"
