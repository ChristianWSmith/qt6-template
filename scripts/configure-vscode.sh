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
    "*.h": "cpp",
    "*.ts": "xml"
  },
  "files.watcherExclude": {
    "${BUILD_DIR}/**": true
  },
  "clangd.enable": true,
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
    },
    {
      "name": "Debug ${APP_NAME} (attach to Container)",
      "type": "lldb",
      "request": "attach",
      "targetCreateCommands": [
          "target create ${BUILD_DIR}/${APP_NAME}"
      ],
      "processCreateCommands": [
          "platform select remote-linux",
          "platform connect connect://localhost:${LLDB_PORT}",
          "process attach -n ${APP_NAME}"
      ],
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
      "command": "${SCRIPT_DIR}/build.sh --build-type Debug --test OFF",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Build ${APP_NAME} (Release)",
      "type": "shell",
      "command": "${SCRIPT_DIR}/build.sh --build-type Release --test OFF",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Build ${APP_NAME} (Debug w/ Tests)",
      "type": "shell",
      "command": "${SCRIPT_DIR}/build.sh --build-type Debug --test ON",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Build ${APP_NAME} (Release w/ Tests)",
      "type": "shell",
      "command": "${SCRIPT_DIR}/build.sh --build-type Release --test ON",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Build ${APP_NAME} in Container (Debug)",
      "type": "shell",
      "command": "${SCRIPT_DIR}/dev-container.sh build --build-type Debug --test OFF",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Build ${APP_NAME} in Container (Release)",
      "type": "shell",
      "command": "${SCRIPT_DIR}/dev-container.sh build --build-type Release --test OFF",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Build ${APP_NAME} in Container (Debug w/ Tests)",
      "type": "shell",
      "command": "${SCRIPT_DIR}/dev-container.sh build --build-type Debug --test ON",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Build ${APP_NAME} in Container (Release w/ Tests)",
      "type": "shell",
      "command": "${SCRIPT_DIR}/dev-container.sh build --build-type Release --test ON",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Run ${APP_NAME} in Container (Debug)",
      "type": "shell",
      "command": "${SCRIPT_DIR}/dev-container.sh debug",
      "group": "build",
      "problemMatcher": []
    },
    {
      "label": "Clean",
      "type": "shell",
      "command": "${SCRIPT_DIR}/clean.sh",
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
        "llvm-vs-code-extensions.vscode-clangd",
        "vadimcn.vscode-lldb",
        "notskm.clang-tidy",
        "jeff-hykin.better-cpp-syntax"
    ]
}
EOF

# Generate keybindings.json
cat > "${VSCODE_DIR}/keybindings.json" <<EOF
{
    "key": "alt+o",
    "command": "clangd.switchSourceHeader"
}
EOF
