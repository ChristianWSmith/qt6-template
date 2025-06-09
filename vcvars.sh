#!/usr/bin/env bash

VCVARS_BAT="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"

# Create a temp Windows batch file
VCVARS_WRAPPER=$(mktemp --suffix=.bat)
cat > "$VCVARS_WRAPPER" <<EOF
@echo off
call "$VCVARS_BAT"
set
EOF

echo "📄 Created wrapper bat file at: $VCVARS_WRAPPER"

# Run the wrapper and capture output
VCVARS_OUTPUT=$(cmd.exe /c "$VCVARS_WRAPPER")

# Clean up temp bat file
rm "$VCVARS_WRAPPER"

# Now continue with processing
echo "$VCVARS_OUTPUT" | tr -d '\r' > vcvars_raw.log

FILTERED=$(grep -E '^(INCLUDE|LIB|LIBPATH|PATH)=' vcvars_raw.log)
echo "$FILTERED" > vcvars_filtered.log

EXPORTS=$(echo "$FILTERED" | sed -E 's|^([^=]+)=(.*)|export \1="\2"|')
echo "$EXPORTS" > vcvars_exports.sh

echo "✅ Export script ready at vcvars_exports.sh"
