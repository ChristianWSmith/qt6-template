#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source "${SCRIPT_DIR}/env.sh"

VSCODE_DIR="${PROJECT_ROOT}/.vscode"

mkdir -p "${VSCODE_DIR}"

# Generate settings.json
cat > "${VSCODE_DIR}/settings.json" <<EOF
{
  "cmake.buildDirectory": "${BUILD_DIR}",
  "editor.formatOnSave": true,
  "editor.tabSize": 4,
  "editor.insertSpaces": true,
  "editor.detectIndentation": false,
  "files.associations": {
    "*.h": "cpp"
  },
  "files.watcherExclude": {
    "${BUILD_DIR}/**": true
  },
  "clangd.arguments": [
    "--compile-commands-dir=${BUILD_DIR}",
    "--clang-tidy"
  ],
  "clangd.path": "$(which clangd)",
  "clang-tidy.buildPath": "${BUILD_DIR}"
}
EOF

# Generate launch.json
cat > "${VSCODE_DIR}/launch.json" <<EOF
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug ${APP_NAME}",
      "type": "lldb",
      "request": "launch",
      "program": "${BUILD_DIR}/${APP_NAME}",
      "args": [],
      "cwd": "${PROJECT_ROOT}"
    },
    {
      "name": "Debug ${APP_NAME} (w/Qt Debug)",
      "type": "lldb",
      "request": "launch",
      "program": "${BUILD_DIR}/${APP_NAME}",
      "args": [],
      "cwd": "${PROJECT_ROOT}",
      "env": {
        "QT_DEBUG_PLUGINS": "1"
      }
    }
  ]
}
EOF

# Generate tasks.json
cat > "${VSCODE_DIR}/tasks.json" <<EOF
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build ${APP_NAME} (Debug)",
      "type": "shell",
      "command": "${PROJECT_ROOT}/scripts/build.sh Debug",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Build MyApp (Release)",
      "type": "shell",
      "command": "${PROJECT_ROOT}/scripts/build.sh Release",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Clean",
      "type": "shell",
      "command": "${PROJECT_ROOT}/scripts/clean.sh",
      "group": "build",
      "problemMatcher": []
    }
  ]
}
EOF

# Generate extensions.json
cat > "${VSCODE_DIR}/extensions.json" <<EOF
{
    "recommendations": [
        "theqtcompany.qt-core",
        "ms-vscode.cmake-tools",
        "theqtcompany.qt-cpp",
        "theqtcompany.qt-ui",
        "llvm-vs-code-extensions.vscode-clangd",
        "vadimcn.vscode-lldb",
        "notskm.clang-tidy",
        "jeff-hykin.better-cpp-syntax"
    ]
}
EOF
