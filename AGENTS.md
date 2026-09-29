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
- Core interfaces: `src/core/` (IModel, IPersistenceProvider)

## Build

- Dependencies: Qt6 (Widgets, LinguistTools), fmt, cxxopts, GTest
- No Vulkan or Qt Concurrent dependency
- Compile definitions centralized via `apply_app_metadata()` CMake function
- `app.env` is the single source of truth for app metadata

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

### EventSystem

- Direct feature relationship: Qt signals/slots.
- Cross-component decoupled application event: EventSystem.
- The EventSystem is reserved for genuinely decoupled events.
- Do not use it for ordinary intra-feature communication or logging.
- Free-function subscribe returns an `events::Subscription` RAII handle.
- QObject subscribe auto-disconnects on receiver destruction.
- Diagnostic output: QLoggingCategory + custom handler.
- Low-level GUI event handling: QObject/QEvent/eventFilter.

The EventSystem provides type-safe, decoupled publish/subscribe.

#### Subscribing

```cpp
// QObject receiver (auto-disconnects on destruction):
events::subscribe<LogEvent>(this, &MyClass::handleEvent);

// Free function (returns RAII Subscription handle):
auto sub = events::subscribe<LogEvent>(myHandler);
// sub must be held alive for the subscription to remain active.
```

#### Publishing

```cpp
events::publish(LogEvent{"message"});
```

#### Lifetime rules

- QObject subscriptions auto-disconnect when the receiver is destroyed.
- Free-function subscriptions require the caller to hold the `Subscription` object.
- Destroying a `Subscription` disconnects the handler.
- Type mismatches are caught at runtime via Q_ASSERT.
- Events are always delivered asynchronously (QueuedConnection).

### Ownership Rules

- QObject with parent → parent owns child (destroyed when parent is destroyed)
- `std::unique_ptr` → explicit exclusive ownership
- Raw pointer / reference → non-owning unless explicitly documented
- Event subscription → lifetime managed by Qt connection mechanism (auto-disconnect on receiver destruction)
- Models take `IPersistenceProvider&` (required, non-owning reference)
- AppMainWindow is the composition root and owns all feature objects via Qt parent-child

### Persistence

- Models own application state.
- Persistence providers own storage mechanics.
- Persistence failures are explicit via `PersistenceResult<T>`.
- Models take `IPersistenceProvider&` (required, not optional).
- `FilePersistenceProvider` uses `QSaveFile` for atomic writes.
- Use `toString(error)` for symbolic error logging.

### Threading

- All QObject-derived application objects are GUI-thread-affine.
- Do not access or mutate them from worker threads.
- Models execute on the GUI thread. Persistence operations are synchronous.
- EventSystem delivery is always queued (Qt::QueuedConnection).
- The EventSystem's BusRegistry mutex protects dispatcher creation, not event delivery.
- Logging is thread-safe (QMutex in message handler).
- No API implies general thread safety merely because it contains a mutex.

### Feature Structure (MVP Convention)

Each feature lives in `src/features/{name}/` with three components:

```
src/features/myfeature/
├── model/        → data + persistence (implements IModel)
├── presenter/    → wiring between model and widget
└── widget/       → UI (no business logic)
```

- **Model** (`model/`) — owns application state, exposes getters/setters, handles persistence via `IPersistenceProvider`
- **Presenter** (`presenter/`) — receives model and widget via constructor, wires signals/slots between them
- **Widget** (`widget/`) — UI only, no business logic, concrete class

`AppMainWindow` constructs all three and wires them together.

### Adding a New Feature

1. Create `src/features/myfeature/model/MyFeatureModel.h` / `.cpp`
2. Create `src/features/myfeature/widget/MyFeatureWidget.h` / `.cpp` + `.ui`
3. Create `src/features/myfeature/presenter/MyFeaturePresenter.h` / `.cpp`
4. Wire in `AppMainWindow` constructor (instantiate presenter, which creates model and widget)
5. Add to `CMakeLists.txt` (auto-discovered via GLOB — touching `CMakeLists.txt` triggers re-glob)

Or use the generator script: `./scripts/generate.sh feature MyFeature`

### Service Registration

Services are registered explicitly in `services::registerAll()` in `src/services/registry/ServiceRegistry.cpp`.

To add a new service:
1. Create the service handler function (must match `void(const EventType&)`)
2. Add `events::subscribe<EventType>(YourService::handle)` to `services::registerAll()`
3. Include the necessary headers

There is no auto-registration macro. Registration is visible and explicit.

### Lifecycle

- AppMainWindow is the composition root.
- Shutdown is synchronous and deterministic.
- `closeEvent` saves state directly — no `processEvents()` pumping.
- QObject destruction follows the ownership tree.

### Non-Goals

This template intentionally does not:

- Use `QtConcurrent` for async operations
- Include Vulkan or GPU-accelerated rendering
- Provide auto-registration macros for services
- Include `IPresenter` or `IWidget` marker interfaces
- Support cross-thread QObject mutation
- Use `REGISTER_SERVICE` or similar registration patterns

### CI

GitHub Actions builds and tests on Linux, Windows, and macOS.
Uses the same Conan/pipenv setup as dev scripts for parity.
See `.github/workflows/ci.yml`.

---

*This documentation is part of the architecture. Treat it as the contract for how the template should be extended.*
