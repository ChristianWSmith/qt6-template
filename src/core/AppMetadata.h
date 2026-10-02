#pragma once

// Build-time application metadata.
//
// These identifiers are NOT declared in any C++ header on purpose. They are
// PUBLIC compile definitions injected by apply_app_metadata() in CMakeLists.txt
// from app.env via scripts/env.sh → Conan → CMake:
//
//   APP_NAME           application display/binary name
//   APP_DESCRIPTION    short description
//   APP_VERSION        version string
//   ORGANIZATION_NAME  QSettings organization
//   APP_ID             reverse-DNS id; used in persistence keys and desktop files
//
// Persistence keys embed APP_ID, e.g. APP_ID ".CounterState" (feature-state
// channel via IPersistenceProvider; window chrome stays on QSettings in
// AppMainWindow — do not merge channels).
// Configure the project through app.env / scripts/build.sh — raw cmake/conan
// without env will fail fast in conanfile.py and CMakeLists.txt.
//
// IDE note: if APP_ID appears undeclared in a model header, ensure
// compile_commands.json comes from a Conan/CMake configure that ran after
// sourcing env (./scripts/build.sh).
