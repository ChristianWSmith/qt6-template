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
./scripts/build.sh --test ON|OFF            # Configure → build → run CTest when ON (default ON)
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
- Tests are discovered via `gtest_discover_tests`
- Run tests: `./scripts/build.sh --test ON` (default) which builds **and executes** the suite via CTest
- CI runs the same path on Linux/Windows/macOS
- Generated test files include `.moc` include at bottom — required for Qt meta-object compilation in test files

## Dependencies

- Managed via Conan (`conanfile.py`), locked with `conan.lock`
- Python tooling via Pipenv (`Pipfile`): conan, aqtinstall, icnsutil, clang-tools
- Lock update: `./scripts/conan-lock-update.sh` (creates per-platform locks then merges)
- Conan profiles: `conan/profiles/{linux,windows,darwin}`

## Key Quirks

- **CMake re-glob**: source discovery uses `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`. If a generator ever misses a new file, touch `CMakeLists.txt` manually — this is a fallback, not the default path.
- **clang-tidy warnings are errors**: `.clang-tidy` sets `WarningsAsErrors: '*'`. Fix all tidy warnings before committing.
- **Qt AUTOUIC/AUTOMOC/AUTORCC**: CMake handles `.ui`, `.moc`, `.qrc` automatically — no manual wrapping needed.
- **Lockfile merges**: Conan lockfile is merged across all 3 platforms. If a version range doesn't satisfy all platforms, you'll need to pin explicitly in `conanfile.py`.
- **Translations**: `qt_add_translations` uses `resources/i18n/` for `.ts` files. Languages configured in `CMakeLists.txt` via `I18N_TRANSLATED_LANGUAGES`. Delete/rename `.ts` files after app name changes.
- **Metadata SSOT**: `app.env` is required. CMake and Conan fail fast with a clear error if `APP_*` variables are missing — there are no silent fallback defaults.
- **Conan compiler profiles**: `conan/profiles/{linux,windows,darwin}` pin compiler versions for dependency resolution. Keep them aligned with the CI runners' actual toolchains when those change.

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
The header `src/events/system/EventSystem.hpp` is the normative contract; this section summarizes it.

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

#### Behavioral contract

| Topic | Guarantee |
|---|---|
| Delivery | Always queued (`Qt::QueuedConnection`). Publish does not invoke handlers on the publisher stack. |
| Destruction | Events targeting destroyed subscribers are dropped. |
| Ordering | Per-connection FIFO via Qt. Cross-subscriber order follows connection creation order. Mid-dispatch subscribe does not see the in-flight event. Publish-during-dispatch is deferred. |
| Type safety | Events must be copy-constructible. Delivery checks meta-type in all builds; debug builds also assert. Mismatch logs `qCritical(appEvent)` and does not deliver. |
| QObject lifetime | Dispatchers and free-function wrapper QObjects are parented to `QCoreApplication` when present (application-owned). |
| Threading | Publish from the GUI thread only. Callbacks run on the receiver's thread via queued delivery. `BusRegistry` mutex protects dispatcher creation, not delivery. |

#### Lifetime rules

- QObject subscriptions auto-disconnect when the receiver is destroyed.
- Free-function subscriptions require the caller to hold the `Subscription` object.
- Free-function `subscribe` is `[[nodiscard]]`; retain the `Subscription` for as long as the handler must remain active.
- Destroying a `Subscription` disconnects the handler (`reset()`); it does **not** depend on `deleteLater()`.
- Application-owned wrappers/dispatchers are reclaimed with `QApplication`.
- EventSystem use requires a running `QApplication` in this template.

### Ownership Rules

- QObject with parent → parent owns child (destroyed when parent is destroyed)
- `std::unique_ptr` → explicit exclusive ownership
- Raw pointer / reference → non-owning unless explicitly documented
- Event subscription → lifetime managed by Qt connection mechanism (auto-disconnect on receiver destruction)
- Models take `IPersistenceProvider&` (required, non-owning reference)
- Presenter holds non-owning raw pointers to model/widget; lifetime is structurally guaranteed by AppMainWindow (presenter does not outlive its dependencies)
- AppMainWindow is the composition root and owns all feature objects via Qt parent-child
- Member declaration order in AppMainWindow is construction order: provider before models; model and widget before presenter
- Ownership mechanism follows object semantics: QObject feature objects use Qt parent-child; pure C++ services may use ordinary C++ lifetime where that improves clarity

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

### Async Policy

The template currently has **no background-worker architecture**. Persistence is intentionally synchronous on the GUI thread. Do not introduce `QThread`, `QtConcurrent`, or other asynchronous machinery merely to avoid synchronous work without first establishing the workload and ownership/threading requirements.

If actual long-running work appears:

1. Identify the operation boundary
2. Define worker ownership
3. Define result delivery
4. Define cancellation
5. Define shutdown behavior
6. Then introduce the smallest appropriate Qt concurrency mechanism

### String Boundary Policy

- **Qt / UI / application-facing APIs:** `QString`
- **Generic event payloads:** `std::string` unless Qt-specific semantics are required

Example: `LogEvent.message` is `std::string` at the bus boundary; the AppLog presenter converts once with `QString::fromStdString` when feeding the Qt model layer. Do not convert a single field across the boundary without establishing a repository-wide policy.

### Logging Categories

| Category | Use |
|---|---|
| `appMain` | Bootstrap / main |
| `appFeature` | Feature widgets and presenters |
| `appPersistence` | Model persistence load/save |
| `appEvent` | EventSystem delivery diagnostics (type mismatch, etc.) |
| `appService` | Service handlers (e.g. ConsoleLogService) |

Diagnostic logging uses `qC*` categories and the custom message handler — **not** the EventSystem.

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

1. Generate the feature: `./scripts/generate.sh feature MyThing`
2. Inspect the generated files (model/presenter/widget + test stub)
3. Wire the feature into `AppMainWindow` (construct model, widget, and presenter — the presenter does **not** create model/widget)
4. Provide required dependencies (`IPersistenceProvider&` into the model)
5. Connect presenter/widget/model signals and slots
6. Add persistence behavior (load in model ctor; save in `AppMainWindow::closeEvent`)
7. Register cross-component events only where justified (EventSystem)
8. Add tests under `tests/features/`
9. Verify shutdown persistence (closeEvent saves state)
10. Build and run the complete test suite: `./scripts/build.sh --test ON`

`generate.sh` is the canonical starting point for new features. Generated output must match the reference implementation (`CounterModel` / `AppLogModel` conventions) — do not "correct" generated code into a different architecture.

### Service Registration

Services are registered explicitly in `services::registerAll()` in `src/services/registry/ServiceRegistry.cpp`.

To add a new service:
1. Create the service handler function (must match `void(const EventType&)`)
2. In `services::registerAll()`, **store** the `events::Subscription` returned by free-function subscribe (process lifetime is appropriate for services)
3. Include the necessary headers

Example:
```cpp
namespace {
events::Subscription g_myServiceSubscription;
}

void registerAll() {
    g_myServiceSubscription =
        events::subscribe<MyEvent>(MyService::handle);
}
```

The free-function `subscribe` overload is `[[nodiscard]]`. Discarding the Subscription disconnects immediately — that is a bug, not an optional style.

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
