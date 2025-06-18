#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/../../scripts/env.sh

# --- Configuration Variables ---
ARCHIVE_NAME="${APP_NAME}-${APP_VERSION}" # Name of the directory inside the tar.gz

# Source paths for your files (adjust these as needed)
# Path to your compiled and patchelf'd MyApp executable
SOURCE_BINARY_PATH="${BUILD_DIR}/${APP_NAME}"
# Directory containing your icon files (e.g., app_icon.svg, app_icon.png)
SOURCE_ICONS_DIR="${ICONS_DIR}"
# Path to your LICENSE file
SOURCE_LICENSE_PATH="${PROJECT_ROOT}/LICENSE"
# Path to your README.md file
SOURCE_README_PATH="${PROJECT_ROOT}/README.md"

# --- Desktop Entry Configuration ---
# We'll create a .desktop.in template, the Makefile will fill in the Exec path
DESKTOP_TEMPLATE_NAME="${APP_ID}.desktop.in" # Note the .in suffix
DESKTOP_ENTRY_NAME="${APP_ID}.desktop"       # The final .desktop filename
DESKTOP_ENTRY_CATEGORY="${APP_CATEGORIES}"
DESKTOP_ENTRY_COMMENT="${APP_DESCRIPTION}"

# --- Makefile Configuration (for installation paths) ---
MAKEFILE_PREFIX="/usr/local" # Default prefix for system-wide install (often /usr/local)
MAKEFILE_USER_PREFIX="\$(HOME)/.local" # User-specific install prefix

# --- Script Logic ---

echo "--- Creating .tar.gz Distribution for ${APP_NAME} v${APP_VERSION} ---"

# 1. Create the root directory for the archive content
TARGET_ROOT_DIR="${PROJECT_ROOT}/${ARCHIVE_NAME}"
mkdir -p "${TARGET_ROOT_DIR}"
echo "Created target directory: ${TARGET_ROOT_DIR}"

# 2. Create the necessary subdirectories
mkdir -p "${TARGET_ROOT_DIR}/bin"
mkdir -p "${TARGET_ROOT_DIR}/share/applications"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/scalable/apps" # For SVG
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/16x16/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/24x24/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/32x32/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/48x48/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/64x64/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/128x128/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/256x256/apps"
mkdir -p "${TARGET_ROOT_DIR}/share/icons/hicolor/512x512/apps"
echo "Created subdirectories."

# 3. Copy the executable
if [ -f "${SOURCE_BINARY_PATH}" ]; then
    cp "${SOURCE_BINARY_PATH}" "${TARGET_ROOT_DIR}/bin/${APP_NAME}"
    chmod +x "${TARGET_ROOT_DIR}/bin/${APP_NAME}"
    patchelf --set-rpath '/usr/lib' "${TARGET_ROOT_DIR}/bin/${APP_NAME}"
    echo "Copied executable: ${APP_NAME} and set RPATH."
else
    echo "Error: Executable not found at ${SOURCE_BINARY_PATH}. Aborting."
    exit 1
fi

# 4. Create the .desktop.in template file (template, not final)
cat << EOF > "${TARGET_ROOT_DIR}/share/applications/${DESKTOP_TEMPLATE_NAME}"
[Desktop Entry]
Type=Application
X-AppVersion=${APP_VERSION}
Name=${APP_NAME}
Exec=@APP_EXEC_PATH@ # Placeholder for the executable path
Icon=${APP_ID}
Categories=${APP_CATEGORIES}
StartupWMClass=${APP_NAME}
EOF
echo "Created desktop template file: ${DESKTOP_TEMPLATE_NAME}"

# 5. Copy icons to their respective FHS locations and resize PNGs
echo "Copying and resizing icons from ${SOURCE_ICONS_DIR}..."

# Check if ImageMagick's 'magick' or 'convert' is available for resizing
if ! command -v magick &> /dev/null && ! command -v convert &> /dev/null
then
    echo "Error: ImageMagick 'magick' or 'convert' command not found. Cannot resize PNG icons."
    echo "Please install ImageMagick (e.g., 'sudo apt install imagemagick' or 'sudo dnf install ImageMagick')."
    exit 1
fi

# Determine which ImageMagick command to use
IMAGEMAGICK_CMD="magick"
if ! command -v magick &> /dev/null; then
    IMAGEMAGICK_CMD="convert"
fi

# Define the standard PNG icon sizes for hicolor theme
PNG_SIZES=(16x16 24x24 32x32 48x48 64x64 128x128 256x256 512x512)

# Source paths for the fixed icons
SOURCE_SVG_ICON="${SOURCE_ICONS_DIR}/app_icon.svg"
SOURCE_PNG_1024_ICON="${SOURCE_ICONS_DIR}/app_icon.png"

# Copy SVG icon
if [ -f "${SOURCE_SVG_ICON}" ]; then
    cp "${SOURCE_SVG_ICON}" "${TARGET_ROOT_DIR}/share/icons/hicolor/scalable/apps/${APP_ID}.svg"
    echo "  Copied SVG: $(basename "${SOURCE_SVG_ICON}")"
else
    echo "Error: SVG icon not found at ${SOURCE_SVG_ICON}. Aborting."
    exit 1
fi

# Resize and copy PNG icons
if [ -f "${SOURCE_PNG_1024_ICON}" ]; then
    for size in "${PNG_SIZES[@]}"; do
        ICON_DIR="${TARGET_ROOT_DIR}/share/icons/hicolor/${size}/apps"
        mkdir -p "${ICON_DIR}" # Ensure directory exists inside the tarball structure
        "${IMAGEMAGICK_CMD}" "${SOURCE_PNG_1024_ICON}" -resize "${size}" "${ICON_DIR}/${APP_ID}.png"
        echo "  Resized and copied PNG: ${APP_ID}.png to ${size}"
    done
else
    echo "Error: 1024x1024 PNG icon not found at ${SOURCE_PNG_1024_ICON}. Cannot resize."
    exit 1
fi

# 6. Copy LICENSE and README.md
if [ -f "${SOURCE_LICENSE_PATH}" ]; then
    cp "${SOURCE_LICENSE_PATH}" "${TARGET_ROOT_DIR}/LICENSE"
    echo "Copied LICENSE."
else
    echo "Warning: LICENSE file not found at ${SOURCE_LICENSE_PATH}. Skipping."
fi

if [ -f "${SOURCE_README_PATH}" ]; then
    cp "${SOURCE_README_PATH}" "${TARGET_ROOT_DIR}/README.md"
    echo "Copied README.md."
else
    echo "Warning: README.md file not found at ${SOURCE_README_PATH}. Skipping."
fi

# 7. Create the Makefile
cat << EOF > "${TARGET_ROOT_DIR}/Makefile"
# Default installation prefix for system-wide install
PREFIX ?= ${MAKEFILE_PREFIX}

# User-specific installation prefix
USER_PREFIX := \$(HOME)/.local

# Directories where files will be installed
BIN_DIR := \$(PREFIX)/bin
SHARE_APP_DIR := \$(PREFIX)/share/applications
SHARE_ICON_DIR := \$(PREFIX)/share/icons/hicolor

# Directories for user-specific install
USER_BIN_DIR := \$(USER_PREFIX)/bin
USER_SHARE_APP_DIR := \$(USER_PREFIX)/share/applications
USER_SHARE_ICON_DIR := \$(USER_PREFIX)/share/icons/hicolor

# Source files to install (relative to the extracted tarball root)
APP_EXECUTABLE := bin/${APP_NAME}
# IMPORTANT: Ensure no trailing spaces on these lines!
DESKTOP_TEMPLATE := share/applications/${DESKTOP_TEMPLATE_NAME}
DESKTOP_FILE_FINAL := ${DESKTOP_ENTRY_NAME}
# Note: APP_ID for icons must match the Icon= field in the .desktop file
# These are the icon base names within the tarball's 'share/icons/hicolor' structure
APP_ICON_NAME := ${APP_ID}
PNG_SIZES := 16x16 24x24 32x32 48x48 64x64 128x128 256x256 512x512

.PHONY: all install uninstall install-user uninstall-user clean

all:

# System-wide installation (requires root privileges if PREFIX is /usr/local or /usr)
install:
	@echo "Installing \$(APP_EXECUTABLE) to \$(BIN_DIR)..."
	mkdir -p "\$(BIN_DIR)"
	cp "\$(APP_EXECUTABLE)" "\$(BIN_DIR)/"
	chmod +x "\$(BIN_DIR)/${APP_NAME}"

	@echo "Creating and installing desktop file to \$(SHARE_APP_DIR)..."
	mkdir -p "\$(SHARE_APP_DIR)"
	# Substitute the executable path in the template and copy
	sed "s|@APP_EXEC_PATH@|\$(BIN_DIR)/${APP_NAME}|g" "\$(DESKTOP_TEMPLATE)" > "\$(SHARE_APP_DIR)/\$(DESKTOP_FILE_FINAL)"

	@echo "Installing icons to \$(SHARE_ICON_DIR)..."
	# Install SVG icon
	mkdir -p "\$(SHARE_ICON_DIR)/scalable/apps"
	cp "share/icons/hicolor/scalable/apps/\$(APP_ICON_NAME).svg" "\$(SHARE_ICON_DIR)/scalable/apps/"
	# Install PNG icons by iterating over sizes
	\$(foreach size,\$(PNG_SIZES),\
		mkdir -p "\$(SHARE_ICON_DIR)/\$(size)/apps" && \
		cp "share/icons/hicolor/\$(size)/apps/\$(APP_ICON_NAME).png" "\$(SHARE_ICON_DIR)/\$(size)/apps/" ;\
	)
	@echo "Updating desktop database..."
	update-desktop-database "\$(SHARE_APP_DIR)" || true # Ignore errors if command not found

	@echo "Updating icon cache..."
	gtk-update-icon-cache -qtf "\$(SHARE_ICON_DIR)" || true # Ignore errors if command not found
	@echo "Installation complete. You may need to log out and back in for icons/desktop entries to appear."


# System-wide uninstallation
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

# User-specific installation (does not require root)
install-user:
	@echo "Installing \$(APP_EXECUTABLE) to \$(USER_BIN_DIR)..."
	mkdir -p "\$(USER_BIN_DIR)"
	cp "\$(APP_EXECUTABLE)" "\$(USER_BIN_DIR)/"
	chmod +x "\$(USER_BIN_DIR)/${APP_NAME}"

	@echo "Creating and installing desktop file to \$(USER_SHARE_APP_DIR)..."
	mkdir -p "\$(USER_SHARE_APP_DIR)"
	# Substitute the executable path in the template and copy
	sed "s|@APP_EXEC_PATH@|\$(USER_BIN_DIR)/${APP_NAME}|g" "\$(DESKTOP_TEMPLATE)" > "\$(USER_SHARE_APP_DIR)/\$(DESKTOP_FILE_FINAL)"

	@echo "Installing icons to \$(USER_SHARE_ICON_DIR)..."
	# Install SVG icon
	mkdir -p "\$(USER_SHARE_ICON_DIR)/scalable/apps"
	cp "share/icons/hicolor/scalable/apps/\$(APP_ICON_NAME).svg" "\$(USER_SHARE_ICON_DIR)/scalable/apps/"
	# Install PNG icons by iterating over sizes
	\$(foreach size,\$(PNG_SIZES),\
		mkdir -p "\$(USER_SHARE_ICON_DIR)/\$(size)/apps" && \
		cp "share/icons/hicolor/\$(size)/apps/\$(APP_ICON_NAME).png" "\$(USER_SHARE_ICON_DIR)/\$(size)/apps/" ;\
	)
	@echo "Updating desktop database..."
	update-desktop-database "\$(USER_SHARE_APP_DIR)" || true
	@echo "Updating icon cache..."
	gtk-update-icon-cache -qtf "\$(USER_SHARE_ICON_DIR)" || true
	@echo "User installation complete. You may need to log out and back in for icons/desktop entries to appear."

# User-specific uninstallation
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
	# No build artifacts to clean in this type of distribution
EOF
echo "Created Makefile."

# 8. Create the .tar.gz archive
echo "Creating archive: ${ARCHIVE_NAME}.tar.gz..."
tar -czvf "${ARCHIVE_NAME}.tar.gz" -C "$(dirname "${TARGET_ROOT_DIR}")" "$(basename "${TARGET_ROOT_DIR}")"
echo "Archive created: ${ARCHIVE_NAME}.tar.gz"

# 9. Clean up the temporary directory
echo "Cleaning up temporary directory: ${TARGET_ROOT_DIR}"
rm -rf "${TARGET_ROOT_DIR}"

echo "--- Distribution creation complete! ---"
echo "Your tar.gz is located at: ./${ARCHIVE_NAME}.tar.gz"
echo "Contents will extract to: ${ARCHIVE_NAME}/"
echo ""
echo "To install system-wide (requires root):"
echo "  tar -xzvf ${ARCHIVE_NAME}.tar.gz"
echo "  cd ${ARCHIVE_NAME}"
echo "  sudo make install"
echo ""
echo "To install for current user only:"
echo "  tar -xzvf ${ARCHIVE_NAME}.tar.gz"
echo "  cd ${ARCHIVE_NAME}"
echo "  make install-user"
