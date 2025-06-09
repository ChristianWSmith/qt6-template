#!/usr/bin/env bash

# === CONFIG ===
VCVARS_BAT='C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat'

echo "Running vcvars batch file..."
# Run the batch file and capture the output of `set`
VCVARS_OUTPUT=$(cmd.exe /c "\"$VCVARS_BAT\" > nul && set")

# Check if we got anything
if [[ -z "$VCVARS_OUTPUT" ]]; then
  echo "❌ No output from vcvars batch file"
  exit 1
fi

echo "✅ Raw output from vcvars:"
echo "$VCVARS_OUTPUT" > vcvars_raw.log
cat vcvars_raw.log | head -n 20
echo "[...output truncated, full log in vcvars_raw.log]"

# Convert Windows line endings and filter only relevant env vars
CLEANED_ENV=$(echo "$VCVARS_OUTPUT" | tr -d '\r' | grep -E '^(INCLUDE|LIB|LIBPATH|PATH)=')

echo "✅ Filtered relevant env vars:"
echo "$CLEANED_ENV" > vcvars_filtered.log
cat vcvars_filtered.log

# Now convert them to export statements
EXPORTS=$(echo "$CLEANED_ENV" | sed -E 's|^([^=]+)=(.*)|export \1="\2"|')

echo "✅ Final export commands:"
echo "$EXPORTS" > vcvars_exports.sh
cat vcvars_exports.sh

eval "$EXPORTS"
