# AGENTS.md

## Project Overview

Qt6 + CMake + Conan C++ GUI application template. Single-app repo (not a monorepo).

## Working With This Repo

**Always prefer the developer scripts.** Do not run raw `conan`, `cmake`, `pipenv`, or `aqtinstall` commands. The scripts in `scripts/` handle environment setup, pipenv invocation, profile selection, lockfile management, and cross-platform details. Running commands directly will bypass these safeguards and often fail silently or produce inconsistent results.

## Single Source of Truth

`app.env` is the canonical source for `APP_NAME`, `APP_VERSION`, `APP_ID`, `QT_VERSION`, etc. All scripts, Conan, and CMake consume it via `scripts/env.sh`. Changing `app.env` flows automatically into builds — no need to touch CMakeLists.txt or conanfile.py for naming/versioning.

## Build Commands

```bash
./scripts/build.sh                          # Release build (default)
./scripts/build.sh --build-type Debug       # Debug build
./scripts/build.sh --clean ON               # Clean before build
./scripts/run.sh                            # Debug-run (builds if needed)
./scripts/run.sh --rebuild ON -- <args>     # Force rebuild, pass args to app
```

All scripts use `pipenv run` under the hood. Pipenv auto-installs on first run.

## Architecture

- Feature-first modular layout: `src/features/<name>/{model,presenter,widget}/`
- Standalone widgets: `src/widgets/<name>/`
- Inter-feature communication: centralized event system in `src/events/`
- Test mirror: `tests/features/<name>/` for feature tests
- Core interfaces: `src/core/` (IModel, IPresenter, IWidget, IPersistenceProvider)

## Code Generation

```bash
./scripts/generate.sh feature MyThing   # Full MV* scaffold
./scripts/generate.sh widget SimplePreview  # Widget only
```

Name must be TitleCase and valid C++ identifier. Generates `.h`, `.cpp`, `.ui` files plus test stub for features. Runs `clang-format` on output if available.

## Testing

- Google Test (`gtest`), linked via `tests/CMakeLists.txt`
- Tests are discovered automatically via `gtest_discover_tests`
- Run tests: `./scripts/build.sh --test ON` (default) then `ctest` in build dir, or use `run.sh --test ON`
- Generated test files include `.moc` include at bottom — required for Qt meta-object compilation in test files

## Dependencies

- Managed via Conan (`conanfile.py`), locked with `conan.lock`
- Python tooling via Pipenv (`Pipfile`): conan, aqtinstall, icnsutil, clang-tools
- Lock update: `./scripts/conan-lock-update.sh` (creates per-platform locks then merges)
- Conan profiles: `conan/profiles/{linux,windows,darwin}`

## Key Quirks

- **CMake re-glob workaround**: build scripts `touch CMakeLists.txt` to force source file re-discovery. If new files aren't picked up, touch it manually.
- **clang-tidy warnings are errors**: `.clang-tidy` sets `WarningsAsErrors: '*'`. Fix all tidy warnings before committing.
- **Qt AUTOUIC/AUTOMOC/AUTORCC**: CMake handles `.ui`, `.moc`, `.qrc` automatically — no manual wrapping needed.
- **Lockfile merges**: Conan lockfile is merged across all 3 platforms. If a version range doesn't satisfy all platforms, you'll need to pin explicitly in `conanfile.py`.
- **Translations**: `qt_add_translations` uses `resources/i18n/` for `.ts` files. Languages configured in `CMakeLists.txt` via `I18N_TRANSLATED_LANGUAGES`. Delete/rename `.ts` files after app name changes.

## VSCode Setup

```bash
./scripts/configure-vscode.sh
```

Rerun after updating `APP_NAME` in `app.env`.

## Qt Installation

Qt is installed locally to `Qt/` directory (gitignored) via `aqtinstall`:

```bash
./scripts/install-qt.sh
```

Auto-triggered by `build.sh` if Qt not found at expected path.

## Architecture Deep Dive

### Event Handling

"When should I use Qt signals versus EventSystem?"

| Scenario | Mechanism |
|---|---|
| Same feature / direct relationship | Qt signal/slot |
| Cross-component domain/application event | `events::publish` / `events::subscribe` |
| Diagnostic logging | `qDebug` / `qInfo` / `QLoggingCategory` (NOT the event system) |
| Low-level Qt event handling | `QObject` / `QEvent` / `eventFilter` |

Qt signals/slots are for tight coupling within a single feature or between closely related objects. The EventSystem (`src/events/`) is for decoupled, cross-component communication — publishing domain events that any subscriber can react to without direct dependencies. Never use the event system for logging or low-level event processing.

### Ownership Rules

- **QObject with parent** → parent owns child (destroyed when parent is destroyed)
- **`std::unique_ptr`** → explicit exclusive ownership
- **Raw pointer / reference** → non-owning unless explicitly documented
- **Event subscription** → lifetime managed by Qt connection mechanism (auto-disconnect on receiver destruction)

When adding members, prefer `std::unique_ptr` for non-QObject resources and parent-child relationships for QObjects. Document any exception where a raw pointer assumes ownership.

### Persistence

- Models own application state (`m_value`, `m_logMessages`)
- `IPersistenceProvider` owns persistence mechanics (file I/O, JSON, fsync)
- `AppMainWindow` controls lifecycle (calls `saveState` during `closeEvent`)
- Persistence errors are explicit (`PersistenceResult<T>` with `PersistenceError` enum)
- "No state exists" (`NotFound`) is distinguishable from "error reading state" (`IoError`, `InvalidData`)

The model owns the data; the provider owns the serialization; the main window owns the save/load lifecycle. Error handling is typed, never ambiguous.

### Threading

- **GUI/QObject state** → GUI thread only
- **EventSystem** → queued delivery (always `Qt::QueuedConnection`)
- **Background work** → only `QtConcurrent` for genuine async I/O (currently only used during file persistence)
- **Shutdown** → deterministic, synchronous on GUI thread

All QObject state mutations happen on the GUI thread. The EventSystem uses queued connections to ensure thread safety. `QtConcurrent` is reserved for I/O-bound work that genuinely benefits from async execution (e.g., file persistence). Shutdown is always synchronous to avoid dangling references.

### Feature Structure (MVP Convention)

Each feature lives in `src/features/{name}/` with three components:

```
src/features/myfeature/
├── model/        → data + persistence (implements IModel)
├── presenter/    → wiring between model and widget
└── widget/       → UI (implements IWidget)
```

- **Model** (`model/`) — owns application state, exposes getters/setters, handles persistence via `IPersistenceProvider`
- **Presenter** (`presenter/`) — receives model and widget via constructor, wires signals/slots between them
- **Widget** (`widget/`) — UI only, no business logic, implements `IWidget` interface

`AppMainWindow` constructs all three and wires them together.

### Adding a New Feature

1. Create `src/features/myfeature/model/MyFeatureModel.h` / `.cpp`
2. Create `src/features/myfeature/widget/MyFeatureWidget.h` / `.cpp` + `.ui`
3. Create `src/features/myfeature/presenter/MyFeaturePresenter.h` / `.cpp`
4. Wire in `AppMainWindow` constructor (instantiate presenter, which creates model and widget)
5. Add to `CMakeLists.txt` (auto-discovered via GLOB — touching `CMakeLists.txt` triggers re-glob)

Or use the generator script: `./scripts/generate.sh feature MyFeature`

### Service Registration

- `REGISTER_SERVICE(function)` macro for auto-registration
- `services::registerAll()` called after `QApplication` construction
- Services should subscribe to events, not construct QObjects during static init

Services are lightweight singletons registered early in the application lifecycle. They subscribe to events rather than taking dependencies on UI components, keeping the dependency graph clean.

---

*This documentation is part of the architecture. Treat it as the contract for how the template should be extended.*
