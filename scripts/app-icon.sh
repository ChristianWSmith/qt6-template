#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

INPUT_FILE="${1:-}"
if [[ -z "${INPUT_FILE}" ]]; then
  echo "Usage: $0 path/to/icon.svg|png|jpg"
  exit 1
fi

if [[ ! -f "${INPUT_FILE}" ]]; then
  echo "Error: File '${INPUT_FILE}' not found."
  exit 1
fi

TMP_DIR="$(mktemp -d)"
BASENAME="app_icon"

# Function to check for tool
require_tool() {
  if ! command -v "$1" &>/dev/null; then
    echo "Error: Required tool '$1' not found in PATH."
    exit 1
  fi
}

require_tool magick

EXT="${INPUT_FILE##*.}"
EXT_LOWER="${EXT,,}"

SOURCE_PNG="${TMP_DIR}/${BASENAME}_source.png"
SOURCE_SVG="${TMP_DIR}/${BASENAME}_source.svg"

warn_if_small() {
  local dims
  dims=$(identify -format "%wx%h" "$1")
  local w=${dims%x*} h=${dims#*x}
  if (( w < 1024 || h < 1024 )); then
    echo "Warning: Input image is smaller than 1024x1024. Consider using a higher-res source for best results."
  fi
}

# Process SVG input
if [[ "${EXT_LOWER}" == "svg" ]]; then
  require_tool rsvg-convert || require_tool inkscape
  cp "${INPUT_FILE}" "${ICONS_DIR}/${BASENAME}.svg"

  if command -v rsvg-convert &>/dev/null; then
    rsvg-convert -w 1024 -h 1024 "${INPUT_FILE}" -o "${SOURCE_PNG}"
  else
    inkscape "${INPUT_FILE}" --export-type=png --export-filename="${SOURCE_PNG}" --export-width=1024 --export-height=1024
  fi
  warn_if_small "${SOURCE_PNG}"

# Process raster input
elif [[ "${EXT_LOWER}" =~ ^(png|jpg|jpeg)$ ]]; then
  magick "${INPUT_FILE}" -resize 1024x1024\> "${SOURCE_PNG}"
  warn_if_small "${INPUT_FILE}"

  # Try to vectorize
  if command -v magick &>/dev/null && command -v autotrace &>/dev/null; then
    magick "${SOURCE_PNG}" -resize 1024x1024! "${TMP_DIR}/resized.png"
    autotrace "${TMP_DIR}/resized.png" \
    --background-color="000000" \
    --output-file="${SOURCE_SVG}" \
    --output-format=svg \
    --color-count=16
    if [[ -f "${SOURCE_SVG}" ]]; then
      cp "${SOURCE_SVG}" "${ICONS_DIR}/${BASENAME}.svg"
      echo "Generated placeholder SVG from raster source."
    else
      echo "Could not generate SVG from raster image. Continuing without it."
    fi
  else
    echo "Skipping SVG generation: magick and/or inkscape not found."
  fi

else
  echo "Unsupported input format: .$EXT"
  exit 1
fi

# Always write out the standard PNG
cp "${SOURCE_PNG}" "${ICONS_DIR}/${BASENAME}.png"

# Generate .ico (multi-size)
magick "${SOURCE_PNG}" -define icon:auto-resize=64,48,32,16 "${ICONS_DIR}/${BASENAME}.ico"

# Generate .icns using Python tool
if pipenv run icnsutil -h &>/dev/null; then
  mkdir -p "${TMP_DIR}/iconset"
  magick "${SOURCE_PNG}" -resize 16x16     "${TMP_DIR}/iconset/icon_16x16.png"
  magick "${SOURCE_PNG}" -resize 32x32     "${TMP_DIR}/iconset/icon_16x16@2x.png"
  magick "${SOURCE_PNG}" -resize 32x32     "${TMP_DIR}/iconset/icon_32x32.png"
  magick "${SOURCE_PNG}" -resize 64x64     "${TMP_DIR}/iconset/icon_32x32@2x.png"
  magick "${SOURCE_PNG}" -resize 128x128   "${TMP_DIR}/iconset/icon_128x128.png"
  magick "${SOURCE_PNG}" -resize 256x256   "${TMP_DIR}/iconset/icon_128x128@2x.png"
  magick "${SOURCE_PNG}" -resize 256x256   "${TMP_DIR}/iconset/icon_256x256.png"
  magick "${SOURCE_PNG}" -resize 512x512   "${TMP_DIR}/iconset/icon_256x256@2x.png"
  magick "${SOURCE_PNG}" -resize 512x512   "${TMP_DIR}/iconset/icon_512x512.png"
  magick "${SOURCE_PNG}" -resize 1024x1024 "${TMP_DIR}/iconset/icon_512x512@2x.png"

  pipenv run icnsutil compose -f "${ICONS_DIR}/${BASENAME}.icns" $(find ${TMP_DIR}/iconset -type f)
else
  echo "Error: 'icnsutil' not available. Run 'pipenv install icnsutil' to enable .icns generation."
fi

rm -rf "${TMP_DIR}"
echo "Icon conversion complete. Output written to: ${ICONS_DIR}"
