# AGENTS.md

## Project Overview

Qt6 + CMake + Conan C++ GUI application template. Single-app repo (not a monorepo).

## Working With This Repo

**Always prefer the developer scripts.** Do not run raw `conan`, `cmake`, `pipenv`, or `aqtinstall` commands. The scripts in `scripts/` handle environment setup, pipenv invocation, profile selection, lockfile management, and cross-platform details. Running commands directly will bypass these safeguards and often fail silently or produce inconsistent results.

## Single Source of Truth

`app.env` is the canonical source for `APP_NAME`, `APP_VERSION`, `APP_ID`, `QT_VERSION`, `I18N_TRANSLATED_LANGUAGES`, etc. All scripts, Conan, and CMake consume it via `scripts/env.sh`. Changing `app.env` flows automatically into Conan/CMake naming and versioning — no need to touch `CMakeLists.txt` or `conanfile.py` for those fields. End-to-end rename is **not** automatic: also rename `resources/i18n/*.ts` files (`APP_NAME`-prefixed) and rerun `./scripts/configure-vscode.sh` (see VSCode Setup and Key Quirks).

## Build Commands

```bash
./scripts/build.sh                          # Release build (default)
./scripts/build.sh --build-type Debug       # Debug build
./scripts/build.sh --clean ON               # Clean before build
./scripts/build.sh --test ON|OFF            # Configure → build → run CTest when ON (default ON)
./scripts/build.sh --update-translations ON|OFF  # lupdate .ts files (default OFF)
./scripts/run.sh                            # Debug-run (builds if needed)
./scripts/run.sh --rebuild ON -- <args>     # Force rebuild, pass args to app
```

Most scripts use `pipenv run` under the hood. The scripts do **not** install the pipenv tool itself: `scripts/env.sh`'s `installPipenv` runs `pipenv install --dev` (installs the Pipfile packages into the project `.venv`; it requires the pipenv tool to already be available). CI installs the tool via `pip install pipenv` before invoking `pipenv install --dev --deploy`.
**Exception:** `build.sh --test ON` runs `ctest` directly after the Conan build (not via `pipenv run`); `run.sh` default `--test` is OFF. Other tool invocations (conan, aqt, clang-format paths) go through pipenv.

**Translations default OFF:** `build.sh` defaults `UPDATE_TRANSLATIONS` to `OFF` so normal builds do not mutate tracked `resources/i18n/*.ts`. Opt in with `--update-translations ON` when you intentionally refresh translation sources.

## Architecture

- Feature-first modular layout (not a flat `src/` tree):

```text
src/
├── main.cpp            # bootstrap; compiled into ${APP_NAME} exe (not _lib)
├── appmainwindow/      # composition root (AppMainWindow)
├── core/               # IModel, IPersistenceProvider (+ PersistenceResult)
├── events/             # EventSystem (system/) + domain event types
├── features/<name>/    # {model,presenter,widget}/ per feature
├── logging/            # qC* categories + message handler
├── platform/           # {core/,theme/} — FilePersistenceProvider, theme.hpp
├── services/           # DemoConsoleLogService + registry/
└── widgets/<name>/     # standalone widgets (generate.sh widget); teaching example lives in examples/widgets/reusable/ (not compiled into _lib)
```

- Test tree: feature tests are flat files under `tests/features/<Name>Test.cpp` (not per-feature directories). Additional suites live under `tests/{events,lifecycle,persistence,services,logging,themes}/`, plus `tests/MemoryPersistenceProvider.h` and `tests/sanity_test.cpp`.
- `src/core/AppMetadata.h` documents that `APP_*` identifiers are PUBLIC compile definitions from `app.env` via `apply_app_metadata()` — not C++ `#define`s in headers.

## Build

- Dependencies: Qt6 (Widgets, LinguistTools; **Tests** for the test binary), fmt, cxxopts, GTest; Conan `build_requires` **ninja**
- No Vulkan or Qt Concurrent dependency
- Compile definitions centralized via `apply_app_metadata()` CMake function (applied to `${APP_NAME}_lib`, `${APP_NAME}`, and `UnitTests`)
- `app.env` is the single source of truth for app metadata
- **`${APP_NAME}_lib` links `Qt6::Widgets` PUBLIC** (public headers include Widgets types) and `fmt::fmt` PRIVATE. Consumers inherit Widgets via the lib; explicit consumer link lines are redundant but harmless.
- **cxxopts** is kept as a third-party Conan CLI dependency alongside **fmt** (dependency-management demonstration). Prefer `QCommandLineParser` / `QString::arg` / `std::format` in **production application code** if Qt-native/C++-standard facilities are a deliberate project choice; do not remove cxxopts/fmt from this template solely because Qt/std have equivalents — they demonstrate Conan dependency management.
- **Theming**: `platform/theme/theme.hpp` loads platform QSS from `:/styles/`. Windows forces the Fusion style and ships `dark.qss`/`light.qss` selected via `QStyleHints::colorScheme()` sampled **once at `setTheme()`** — no live `colorSchemeChanged` re-apply (runtime OS theme switches need an app restart). Linux/macOS paths never sample `colorScheme()`. Linux/macOS `base.qss` and `custom.qss` may be empty — empty means **Qt default styling**. `custom.qss` is the primary application override extension point. Do not add placeholder CSS just to make files non-empty. QSS open failures log under `appMain`.
- **cxxopts** is linked only to the `${APP_NAME}` executable (CLI in `main.cpp`); it is not a `${APP_NAME}_lib` dependency.
- **`configureLogLevel`** is declared in `src/logging/logging.h` and defined in `src/logging/logging.cpp` (not `main.cpp`). Unknown tokens leave Qt rules unchanged.
- **Architecture boundary checks** run in `scripts/build.sh` (not CI-only) and as CTest `ArchitectureBoundaries` on UNIX. The script is a **heuristic source-pattern check** (grep/find), not a C++ dependency graph — a green run is evidence, not proof of full conformance. Rule numbering matches `scripts/check-architecture-boundaries.sh`:
  1. Feature widgets and standalone `src/widgets/` must not include model headers (`model/` path segment or `*Model.h` basename; basename rule excludes `IModel.h`).
  1w. Feature/standalone widgets must not include `events/`, `EventSystem`, `platform/`, or `core/` headers (UI stays UI-only; **logging is allowed** — reference widgets include it).
  2. `src/logging/` must not reference the EventSystem surface (`events::`, `LogEvent`, `DemoLogEvent`, `EventSystem`, `BusRegistry`, `EventDispatcher`, `Subscription`) and must not `#include` `events/` headers.
  3. Feature models must not include widget headers (`widget/` path or `*Widget.h` basename).
  4. Feature models must not include EventSystem/`events/` headers.
  5. Infrastructure must not depend on features: `src/{events,services,platform}` must not include `features/`.
  6. `src/core` must not include `widgets/`, `events/`, `features/`, or `platform/`.
  
  Exemptions: presenters, `src/appmainwindow/`, `src/main.cpp`, `tests/`. Allowed directions (not violations): `src/events` → `src/logging`; `src/services` → `src/events` + `src/logging`; `src/platform` → `src/core` + `src/logging`; `src/features/*/model` → `src/core` + `src/logging`; `src/features/*/widget` → `src/logging`.
- **UnitTests QSS embedding:** `tests/CMakeLists.txt` derives the QSS list from `resources/resources.qrc` via `cmake/ParseQrcStyles.cmake` (AUD-121) and embeds those files with `qt_add_resources` — the production qrc remains the single source of truth for which style files ship. Do not hardcode a parallel QSS list. `cmake/VerifyProductionResources.cmake` regression-checks the **production** executable (not UnitTests).
- **CMake AUTOGEN_BUILD_DIR:** `CMakeLists.txt` sets `AUTOGEN_BUILD_DIR` explicitly on `${APP_NAME}_lib` and exposes `${AUTOGEN_BUILD_DIR}/include` as an INTERFACE include. Do not "simplify" this to a `$<TARGET_PROPERTY:...>` generator expression — generator-expression forms evaluate empty in consumer compile lines (AUD-110).
- **ReusableWidget** lives at `examples/widgets/reusable/` (Wave 8 relocation). It is an intentional teaching example of the standalone-widget convention (deleted copy/move, `std::unique_ptr<Ui::ReusableWidget>`, signal/slot placeholders). It is **not** compiled into `${APP_NAME}_lib` and is not instantiated by the sample app. Copy the pattern into `src/widgets/<name>/` when scaffolding a real standalone widget. `generate.sh widget` remains the canonical generator for new widgets.
- **`build.sh --test ON`** also runs the production binary `--smoke-test` headless after ctest. The binary path is resolved via `scripts/lib/resolve-app-path.sh`: Windows prefers the Ninja single-config layout at `${BUILD_DIR}/${APP_NAME}.exe` first, then falls back to multi-config `${BUILD_DIR}/${CMAKE_BUILD_TYPE}/${APP_NAME}.exe` (Visual Studio). Missing smoke binary: **hard-fail** under CI or when `BUILD_TESTING=ON`; otherwise warn and skip. Packaging scripts (`.github/scripts/*`) require `--smoke-test` in `main.cpp`.

### Build targets and facts (verified)

- **Targets:** `${APP_NAME}_lib` (STATIC, application sources minus `main.cpp`) → `${APP_NAME}` executable (resources + `src/main.cpp`) → `UnitTests`
- **`main.cpp` is compiled into the executable target**, not `${APP_NAME}_lib`. Do not add `main.cpp` to the library sources.
- **Test binary name is hardcoded** as `UnitTests` in `tests/CMakeLists.txt`. There is no `UT_NAME` CMake variable or indirection.
- **`BUILD_TESTING`:** `scripts/build.sh` defaults it to `ON`; `conanfile.py` CMake configure also defaults `BUILD_TESTING` to `ON` if unset. `run.sh` defaults `--test OFF`.
- **`gtest_discover_tests(UnitTests)`** runs in default **POST_BUILD** discovery mode (plus explicit `DISCOVERY_TIMEOUT 30`). Tests are discovered after the test binary is built.
- **Tests link `Qt6::Test`** (plus Core/Widgets from the root-scope `find_package`; `tests/CMakeLists.txt` requests `Qt6::Test` only). Custom `tests/main.cpp` provides `main()` (QApplication + gtest init) — do not link `gtest_main`.
- **`ctest` runs directly** from `build.sh` (`ctest --test-dir "${BUILD_DIR}" --output-on-failure`) with `QT_QPA_PLATFORM` defaulting to `offscreen` when tests are ON. The default is exported **before** the Conan/CMake build so `gtest_discover_tests` POST_BUILD discovery also runs headless.
- **`install()` covers the executable target only** (BUNDLE/LIBRARY/RUNTIME destinations). There is no `export()` / package-config install. Platform packaging (AppImage, installers, `.app`) is done by `.github/scripts/{linux-bundle-*.sh,mac-bundle-app.sh,windows-bundle.sh}` in CI.
- **CI runs the generator smoke test** via `scripts/test-generator.sh` (generates `GenSmokeProbe`, checks conventions, builds with `--test ON`, cleans up). See `.github/workflows/ci.yml`.
- **Conan dependency layout:** `requires` = fmt + cxxopts; `build_requires` = ninja; `test_requires` = gtest. `conan.lock` may also record profile-driven or transitive entries (e.g. Linux `xorg/system`, pkgconf/meson/cmake) beyond the declared recipe layout — that superset is expected Conan lock behavior, not a defect.
- **fmt lock example:** pin range in `conanfile.py` is `fmt/[>=12.0.0 <13]` (lock currently resolves 12.x). Use that range in docs/examples — do not cite older ranges.
- **Dockerfile is toolchain-only** (compiler, cmake, pipenv, X11/build deps). It does **not** install a parallel system Qt; Qt still comes from the local `Qt/` tree via `scripts/install-qt.sh` (aqtinstall). Do not document a Docker Qt path.
- **Dockerfile toolchain alignment:** Ubuntu 26.04 with explicit `gcc-15`/`g++-15` plus `update-alternatives` so plain `gcc`/`g++` resolve to major 15 — matching `conan/profiles/linux` `compiler.version=15`. CI Linux installs the profile major on the runner and runs a **Compiler profile guard** step that fails when the runner compiler major ≠ profile major (Conan does not verify this itself). `conan/profiles/{linux,darwin}` also set `tools.cmake.cmaketoolchain:generator=Ninja` (Windows already did); `conan/profiles/darwin` pins apple-clang 21 + armv8 for macos-latest; `conan/profiles/windows` pins msvc 194 (v143 compat on VS 2026 images) deliberately.
- **Conan profile pin check:** `scripts/check-conan-profile-toolchain.sh` compares every `conan/profiles/*` `compiler.version` pin against the detected host compiler (linux/darwin/windows). CI runs it as a **non-fatal** step (`continue-on-error: true`) while the linux pin intentionally targets incoming ubuntu-26.04 / gcc 15. Flip to fatal once runner migration is validated. Complements (does not replace) the Linux-only fatal Compiler profile guard.
- **aqtinstall is pinned to a git commit** (`076e1659…` in `Pipfile`), not floating `master` and not PyPI `3.3.0`. PyPI 3.3.0 cannot install Qt 6.11+ on Windows (Qt uses arch-specific repo folders such as `qt6_6111/qt6_6111_msvc2022_64/`; aqt PR #1000 adds that support; the PR merged 2026-03-24 but is still unreleased on PyPI as of 2026-10-02). Switch to a PyPI release when a published release **contains PR #1000** (version may be 3.3.1+ rather than 3.4.0 — check the changelog).

## Code Generation

```bash
./scripts/generate.sh feature MyThing   # Full MV* scaffold
./scripts/generate.sh widget SimplePreview  # Widget only
```

Name must start with an uppercase letter and be a valid C++ identifier (regex `^[A-Z][A-Za-z0-9]*$`; strict PascalCase is not enforced). Generates `.h`, `.cpp`, `.ui` files plus test stub for features. Runs `clang-format` on output if available.

## Testing

- Google Test (`gtest`), linked via `tests/CMakeLists.txt` into a single binary target **`UnitTests`** (hardcoded name; no `UT_NAME`)
- `tests/CMakeLists.txt` requests `Qt6::Test` (Core/Widgets targets come from the root-scope `find_package`); custom `tests/main.cpp` supplies `main()` — do not link `gtest_main`
- Tests are discovered via `gtest_discover_tests` (POST_BUILD default mode)
- Run tests: `./scripts/build.sh --test ON` (default) which builds **and executes** the suite via CTest (ctest runs outside pipenv; `QT_QPA_PLATFORM` defaults to `offscreen` when tests are ON, applied before the build so test discovery is headless too)
- CI runs the same path on Linux/Windows/macOS, plus `scripts/test-generator.sh`
- Generated test files include a `.moc` include at the bottom. It is **required** when the test `.cpp` defines `Q_OBJECT` (e.g. `tests/events/EventsTest.cpp`); feature fixtures without `Q_OBJECT` compile an empty moc — the include is harmless convention there, not a hard requirement for every test file.

**Test-mode isolation contract:** `tests/main.cpp` enables Qt test mode (`QStandardPaths::setTestModeEnabled(true)`) and redirects QSettings UserScope paths to a temp dir **before** any test constructs persistence providers or windows. Invalid `QTemporaryDir` **aborts** the test bootstrap (isolation is not optional). If `QT_QPA_PLATFORM` is unset, `tests/main.cpp` defaults it to `offscreen` for IDE/headless builds. New test targets must preserve this contract — tests never write real `AppDataLocation` data or the developer's organization/application settings.

### Operational Recipes

**Run a single CTest test** (after a normal test build):

```bash
# Via ctest (uses the build tree; needs QT_QPA_PLATFORM for GUI tests):
QT_QPA_PLATFORM=offscreen ctest --test-dir build -R 'CounterTest\.' --output-on-failure

# Or run the test binary directly:
QT_QPA_PLATFORM=offscreen ./build/tests/UnitTests --gtest_filter='CounterTest.*'
```

`./scripts/build.sh --test ON` builds and runs the **full** suite. Direct `ctest` requires a prior successful build and assumes system `cmake` is on `PATH` (ctest is invoked outside pipenv).

**Add a logging category:**

1. In `src/logging/logging.h`: add `Q_DECLARE_LOGGING_CATEGORY(appMyFeature)`
2. In `src/logging/logging.cpp`: add `Q_LOGGING_CATEGORY(appMyFeature, "app.myfeature")`
3. Use `qCDebug(appMyFeature) << ...` from the owning layer.

New categories automatically inherit wildcard filter rules from `configureLogLevel` (e.g. `*.debug=true`). Filtering is by category string rules, not by per-category CLI flags. The custom `messageHandler` prints the category name in each line when `QT_MESSAGELOGCONTEXT` is defined.

**Inject a persistence provider into tests** (do not write real app data):

```cpp
MemoryPersistenceProvider provider;  // tests/MemoryPersistenceProvider.h
AppMainWindow window(provider);      // injected ctor: IPersistenceProvider&
// Provider must outlive the window. Not reparented. See Ownership Rules.
```

Use `failNextSave(...)` / `failNextLoad(...)` on the double for one-shot failure injection. Feature fixtures construct `Model(provider, nullptr)` directly without a window. `seedRaw(...)` injects arbitrary bytes for InvalidData taxonomy tests (`tests/persistence/MemoryPersistenceProviderTest.cpp`). Contract tests for the double (NotFound / round-trip / one-shot injection) live in `tests/persistence/MemoryProviderContractTest.cpp`.

## Dependencies

- Managed via Conan (`conanfile.py`), locked with `conan.lock`
- Python tooling via Pipenv (`Pipfile`): conan, aqtinstall, icnsutil, clang-tools
- Lock update: `./scripts/conan-lock-update.sh` (creates per-platform locks then merges)
- Conan profiles: `conan/profiles/{linux,windows,darwin}`
- `conan.lock` is a resolved superset of the recipe: it can include profile package-manager entries and transitive build tools. Prefer regenerating via the lock-update script over hand-editing.

## Key Quirks

- **CMake re-glob**: source discovery uses `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`. If a generator ever misses a new file, touch `CMakeLists.txt` manually — this is a fallback, not the default path.
- **clang-tidy (editor-side)**: `.clang-tidy` sets `WarningsAsErrors: '*'` for editor/clangd use via `./scripts/configure-vscode.sh`. CI does not currently run clang-tidy as a build step. Keep generated code free of new tidy findings; preserve intentional NOLINT annotations.
- **Qt AUTOMOC/AUTOUIC/AUTORCC**: `qt_standard_project_setup()` enables AUTOMOC and AUTOUIC only — **not** AUTORCC. This project sets `CMAKE_AUTORCC ON` in `CMakeLists.txt` so `resources.qrc` listed on the executable is actually processed. Without AUTORCC, a bare `.qrc` source is a silent no-op (no resources embedded). Do not remove the AUTORCC line; tests that embed QSS via `qt_add_resources` do not substitute for production embedding.
- **Lockfile merges**: Conan lockfile is merged across all 3 platforms. If a version range doesn't satisfy all platforms, you'll need to pin explicitly in `conanfile.py`. The merged lock may contain additional profile/transitive packages; do not "clean" the lock without re-running `./scripts/conan-lock-update.sh` and re-validating CI.
- **Translations**: `qt_add_translations` uses `resources/i18n/` for `.ts` files. Languages are configured in **`app.env`** (`I18N_TRANSLATED_LANGUAGES`) and threaded through Conan → CMake `qt_standard_project_setup`. Default builds do **not** update `.ts` files (`UPDATE_TRANSLATIONS=OFF`); opt in via `./scripts/build.sh --update-translations ON`. Delete/rename `.ts` files after app name changes (`resources/i18n/${APP_NAME}_<lang>.ts`).
- **Metadata SSOT**: `app.env` is required. CMake and Conan fail fast with a clear error if `APP_*` variables are missing — there are no silent fallback defaults.
- **Conan compiler profiles**: `conan/profiles/{linux,windows,darwin}` pin compiler versions for dependency resolution. Linux/darwin also set `tools.cmake.cmaketoolchain:generator=Ninja` (Windows already did). `scripts/check-conan-profile-toolchain.sh` compares pins to the detected host compiler (CI step is non-fatal while the linux pin intentionally targets incoming ubuntu-26.04 / gcc 15). Darwin pin is apple-clang 21 + armv8 for macos-latest. Windows msvc 194 is a deliberate v143 compat pin on VS 2026 images.
- **CMake presets**: Conan CMakeToolchain generates `build/CMakePresets.json` and `CMakeUserPresets.json` (e.g. `cmake --preset conan-release`). Prefer `./scripts/build.sh`; presets are an optional IDE/parallel entry, not a replacement for `app.env` SSOT.

### Deferred (Wave 8 — revisit only if requirements change)

Per AUDIT-REPORT-2 migration Wave 8, these are **intentionally not done** in the remediation pass:

| Item | When to revisit |
|---|---|
| Multi-target CMake layering (`_lib` split) | Template evolves into a larger application with real layering pressure |
| `PersistenceResult` → `std::expected` | C++23 pin adopted (dedicated change) |
| pipenv → uv | CI times or onboarding pain material (tooling only) |
| EventSystem cross-thread delivery | Measured cross-thread workload after Async Policy escalation |
| Hand-rolled root `CMakePresets.json` | Conan already generates presets; do not fight the toolchain |

Wave 8 items that **were** applied: ReusableWidget teaching artifact moved to `examples/widgets/reusable/` (not compiled into `_lib`); QObject subscribe returns `Subscription` (AUD-105).

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
- Both subscribe overloads return an `events::Subscription` RAII handle.
- **Free-function subscribe:** caller must retain the handle; discarding it disconnects immediately (`[[nodiscard]]`). Connection context is `QCoreApplication` — no per-subscription wrapper QObject is allocated. `Subscription` holds a `QMetaObject::Connection` only.
- **QObject-receiver subscribe:** returns a **receiver-lifetime** `Subscription` (`Subscription::forReceiverLifetime`). Discarding the handle does **not** disconnect — connection lifetime follows the receiver (Qt auto-disconnect on receiver destruction). `reset()` still disconnects for early unsubscribe.
- Delivery is always queued; publishers do not synchronously execute subscribers.
- Diagnostic output: QLoggingCategory + custom handler.
- Low-level GUI event handling: QObject/QEvent/eventFilter.

The EventSystem provides type-safe, decoupled publish/subscribe.
The header `src/events/system/EventSystem.hpp` is the normative contract; this section summarizes it.

#### Public API boundary

- Application code should use `events::publish` and `events::subscribe` (free-function and QObject overloads). These are the supported public-facing entry points.
- `BusRegistry` also exposes equivalent static `publish`/`subscribe` operations; consumers should use the free-function facade. `BusRegistry::dispatcher<T>()` is a **private** implementation detail — do not call it or treat it as public API.
- Both free-function `subscribe` overloads are `[[nodiscard]]`. For **free-function** handlers, discarding the returned `Subscription` disconnects immediately. For **QObject receivers**, discarding is normal — the receiver-lifetime handle does not disconnect on destruction; `reset()` is the early-unsubscribe path.

#### Subscribing

```cpp
// QObject receiver — returns a receiver-lifetime Subscription.
// Discarding the handle does NOT disconnect (lifetime follows the receiver):
auto qsub = events::subscribe<DemoLogEvent>(this, &MyClass::handleEvent);
// Optional: qsub.reset() for early unsubscribe while the handle is held.

// Free function — returns RAII Subscription handle; MUST be retained:
auto sub = events::subscribe<DemoLogEvent>(myHandler);
// sub must be held alive for the subscription to remain active.
// Discarding `sub` disconnects immediately.
```

- QObject-receiver subscriptions are **receiver-lifetime**: connection follows the receiver; discarding the handle does not disconnect. Retain the handle only if you need `reset()` for early unsubscribe.
- Free-function subscriptions belong to application lifetime. Connection context is `QCoreApplication` — there is no per-subscription wrapper QObject. `Subscription` holds only the `QMetaObject::Connection`.
- Free-function handlers execute on the application thread under the GUI-thread policy (`QCoreApplication` context + queued delivery).

#### Publishing

```cpp
events::publish(DemoLogEvent{"message"});
```

- Publishing is a GUI-thread operation under the current architecture. **All builds** reject wrong-thread publish/subscribe at runtime (`qCCritical(appEvent)` + no-op); debug builds also assert.
- Recursive publication is queued rather than immediate.

#### Behavioral contract

| Topic | Guarantee |
|---|---|
| Delivery | Always queued (`Qt::QueuedConnection`). Publishers do not synchronously execute subscribers; handlers never run on the publisher's stack. |
| Destruction | Events targeting destroyed subscribers are dropped. Destroyed QObject receivers are automatically disconnected. |
| Ordering | Per-connection FIFO via Qt. Cross-subscriber order follows Qt connection-order semantics — an implementation/Qt-derived property, not a stronger application guarantee unless locked by a regression test. Mid-dispatch subscribe does not see the in-flight event. Publish-during-dispatch is deferred. Recursive publication is queued rather than immediate. |
| Type safety | Event types must be default-constructible, copy-constructible, and copy-assignable (EventType concept). Delivery checks meta-type in all builds; debug builds also assert. Mismatch logs `qCritical(appEvent)` and does not deliver. |
| Free-function subscribe | No per-subscription wrapper QObject is allocated. Connection context is `QCoreApplication`. `Subscription` holds `QMetaObject::Connection` only. Subscriptions belong to application lifetime; free-function handlers execute on the application thread under the GUI-thread policy. Discarding a free-function handle disconnects immediately. |
| QObject subscribe | Returns a receiver-lifetime `Subscription` (`forReceiverLifetime`). Discarding the handle does **not** disconnect; connection lifetime follows the receiver. `reset()` still disconnects for early unsubscribe. |
| QObject lifetime | Dispatchers are parented to `QCoreApplication` when present (application-owned). Free-function connection context is also `QCoreApplication`. |
| Threading | Publish/subscribe are GUI-thread-only (template policy; no worker threads). The policy is enforced at runtime in **all builds** (wrong-thread calls log `qCritical(appEvent)` and no-op) and with `Q_ASSERT` in debug builds. Callbacks run on the receiver's thread via queued delivery. Free-function handlers execute on the application thread under the GUI-thread policy. `BusRegistry` mutex protects dispatcher creation, not event delivery. The bus is not a general cross-thread synchronization mechanism. |

#### Implementation constraints (Qt moc, exceptions, reset)

- **moc constraint:** `EventDispatcher<T>` cannot use `Q_OBJECT` because Qt moc does not support `Q_OBJECT` in class templates. The non-template base carries the Qt signal using **QVariant transport**. A typed signal directly on `EventDispatcher<T>` is not viable under Qt 6 moc — do not "simplify" the bus into code that does not build. The runtime meta-type check is intentional defense-in-depth at that type-erasure boundary.
- **Exception policy:** event handlers must not throw. Delivery uses Qt queued connections; exceptions from handlers follow Qt slot semantics (undefined unless handled in the handler). The EventSystem does not define a custom exception framework — do not add a catch layer in the bus.
- **Subscription::reset() semantics:** `reset()` disconnects the connection for **future** delivery. Per Qt queued-connection semantics, events already posted to a still-alive receiver's queue may still be delivered after `reset()`. New deliveries are prevented. Destroying a `Subscription` does not depend on `deleteLater()`.

#### Lifetime rules

- QObject-receiver subscriptions are **receiver-lifetime**: connection follows the receiver; discarding the handle does not disconnect. Qt tears the connection down when the receiver QObject is destroyed. Retain the handle only if you need `reset()` for early unsubscribe.
- Free-function subscriptions belong to application lifetime; connection context is `QCoreApplication`.
- Free-function subscriptions require the caller to hold the `Subscription` object. Retain the handle for as long as the handler must remain active.
- Free-function `subscribe` is `[[nodiscard]]`; discarding the `Subscription` disconnects immediately.
- Destroying a `Subscription` disconnects the handler for future deliveries (`reset()`); it does **not** depend on `deleteLater()`. See Implementation constraints for in-flight queued events. For receiver-lifetime handles, destruction does not disconnect — only `reset()` (or receiver destruction) does.
- `Subscription` owns its connection lifetime (RAII) and holds only the `QMetaObject::Connection`. Free-function handles disconnect on destruction; QObject-receiver handles (`forReceiverLifetime`) do not.
- Application-owned dispatchers are reclaimed with `QApplication` (parented to `QCoreApplication` when present).
- The `BusRegistry` dispatcher map is cleared on `QCoreApplication::aboutToQuit`, before Qt-owned dispatcher objects are destroyed. The registry never deletes dispatchers. `aboutToQuit` also sets `quitFired_`; publish/subscribe after shutdown refuse to recreate dispatchers. Quit-hook state re-arms when a new `QCoreApplication` instance appears.
- EventSystem use requires a running `QApplication` in this template.

#### Production status of DemoLogEvent / AppLog

- The stock application does **not** publish `DemoLogEvent` in production code. Absence of production publishers is intentional pedagogy, not an unfinished feature. Types are named `Demo*` so code shape does not imply a live production flow.
- Production **subscription** wiring exists: `AppLogPresenter` (QObject receiver, constructed by `AppMainWindow`; stores the receiver-lifetime `Subscription` for optional `reset()`) and `DemoConsoleLogService` (free-function, registered in `services::registerAll()`).
- AppLog's real feature data path is model signals (`logChanged` / `logCleared`); the presenter always pushes **full retained state** via `AppLogWidget::setLogMessages` (AUD-116) — the `LogDelta` payload is not forwarded to the view. The DemoLogEvent bus path is demonstration wiring without a production feed.
- This is an **architectural demonstration** of EventSystem subscription, not a live end-to-end event flow.
- Diagnostic logging (`qC*` + message handler) never uses the EventSystem.
- Applications that need cross-component events publish their own domain events from semantically honest sites (feature actions, lifecycle, etc.) — do not route diagnostic logs onto the bus.
- `AppLogModel` persistence demonstrates **feature-state persistence** via `IPersistenceProvider`. It is **not** a recommendation that production diagnostic logs be persisted as application state.

### Ownership Rules

- QObject with parent → parent owns child (destroyed when parent is destroyed)
- `std::unique_ptr` → explicit exclusive ownership
- Raw pointer / reference → non-owning unless explicitly documented
- Event subscription → lifetime managed by Qt connection mechanism (auto-disconnect on receiver destruction)
- `Subscription` owns its connection lifetime (RAII); free-function subscribe requires retaining the returned handle
- Application-owned dispatchers are reclaimed with `QApplication` (parented to `QCoreApplication` when present); the registry map is cleared on `aboutToQuit` before those objects are destroyed
- Models take `IPersistenceProvider&` (required, non-owning reference)
- Presenters hold non-owning `QPointer` refs to model/widget; presenter slots null-guard before calling into them; presenter destructors must not dereference them
- Member declaration order in AppMainWindow is construction order only — not a Qt destruction-order guarantee
- Qt signals/slots auto-disconnect when either QObject (sender or receiver) is destroyed
- AppMainWindow is the composition root and owns all feature objects via Qt parent-child
- Persistence provider ownership is encoded in **two `AppMainWindow` constructors**:
  - **Default:** `AppMainWindow(QWidget *parent = nullptr)` constructs `FilePersistenceProvider(this)` — a QObject child of the window (Qt parent-child ownership). Provider and models are **sibling** Qt children; there is **no** sibling destruction-order guarantee. Outliving is **convention-only** on this path: model destructors must not touch the provider.
  - **Injected:** `AppMainWindow(IPersistenceProvider &provider, QWidget *parent = nullptr)` is caller-owned, is **not** reparented, and **must outlive** `AppMainWindow`. Outliving is **structural** when the caller declares the provider before the window (C++ stack destruction order — reverse of declaration). Production `main()` uses this path: `FilePersistenceProvider provider;` then `AppMainWindow mainWindow(provider);`. This is the tested persistence seam (`MemoryPersistenceProvider` in lifecycle tests). The reference type encodes non-null; the outliving rule remains contractual on the default path.
- **`AppMainWindow` is stack-allocated in `main()` — do not set `Qt::WA_DeleteOnClose`** (would double-destroy the window). `closeEvent` intentionally calls `hide()` first.
- **`AppMainWindow` keeps a `QList<IModel*>` (`m_models`) for composition-root shutdown persistence.** Models implement `IModel`. The list is non-owning (Qt parent-child owns the models); it only drives `closeEvent`'s polymorphic `saveState()` loop. Shutdown persistence is driven by `registerModel(IModel*)` in `AppMainWindow::constructFeatures()` — the sole append site (non-null assert + `m_models << model`). There is **no** hardcoded Counter+AppLog census assert. Extension: call `registerModel` for each new feature model; add its widget in `finishConstruction()`.
- **Model destructors and teardown-reachable code must not dereference `IPersistenceProvider&`** — persistence I/O is only valid during `loadState`/`saveState` while the provider is guaranteed alive.
- Ownership mechanism follows object semantics: QObject feature objects use Qt parent-child; pure C++ services may use ordinary C++ lifetime where that improves clarity

### Persistence

Persistence failures have three distinct layers — do not conflate them:

| Layer | What it is | Who owns it |
|---|---|---|
| **Typed error representation** | `PersistenceResult<T>` / `PersistenceError` returned from `loadState`/`saveState` | Models forward results; composition root observes them |
| **Diagnostic logging** | `qCWarning`/`qCDebug` on operational failures | **`FilePersistenceProvider` only** — models do not duplicate persistence diagnostics |
| **Application-level handling** | What the app does with a failed save/load | Template default is log-and-continue at shutdown; applications may surface failures to users if their requirements demand it — the template adds no UI error path |

- Models own application state.
- Persistence providers own storage mechanics.
- Persistence failures are explicit via `PersistenceResult<T>`.
- Models take `IPersistenceProvider&` (required, not optional).
- **`IModel::saveState()` returns `PersistenceResult<void>`.** Models forward the provider result; they do not flatten it. `NotFound` is first-run state (silent). Operational load/save failures are logged by the provider; models branch on the result without emitting duplicate persistence diagnostics.
- **`AppMainWindow` holds `QList<IModel*>` and loops every model in `closeEvent`.** The composition root observes results; the default shutdown policy is **log-and-continue** (never blocks close on persistence failure).
- `FilePersistenceProvider` commits via `QSaveFile` atomic replace (not power-loss durability). `QSaveFile::commit()` failures are reported as `PersistenceError::CommitError` (atomic replace did not complete); Qt does not surface fsync errors from `commit()`. Persistence-failure logging is owned by the provider; models forward results without logging save errors.
- Use `toString(error)` for symbolic error logging.
- Persistence keys embed the `APP_ID` compile definition from `app.env` (e.g. `APP_ID ".CounterState"`, `APP_ID ".AppLogState"`). Keys are strings passed to the provider; do not invent a separate key registry.
- Storage path is `QStandardPaths::AppDataLocation + "/" + key + ".json"`. On Windows, `AppDataLocation` is the **Roaming** path (`AppLocalDataLocation` would be Local).

#### Consumer-facing consequences

- `closeEvent` never blocks shutdown on save failure (log-and-continue; see the composition-root bullet above). QSettings chrome (geometry/state) save is a separate channel from feature-state persistence, and failures there are also non-blocking at shutdown.
- `QSaveFile` atomic replace is not power-loss durability: `PersistenceError::CommitError` means `QSaveFile::commit()` failed (the atomic replace did not complete); Qt does not surface fsync errors from `commit()`.
- At the **provider** boundary, `FilePersistenceProvider` distinguishes failure kinds: missing file → `NotFound` (first-run); exists-but-unreadable → `IoError`; damaged JSON / non-object → `InvalidData`; failed atomic replace → `CommitError`.
- Corrupt persisted state (`InvalidData`) is **quarantined** by `FilePersistenceProvider` (file renamed to `*.corrupt`) before models see defaults. The next `saveState()` writes fresh defaults; the corrupt bytes are preserved on disk for recovery, not silently overwritten in place. The quarantine rename closes the `QFile` handle first (Windows cannot rename an open file).
- At the **model** boundary, `IModel::loadState()` is `void` by design: `NotFound` is silent first-run, and operational load errors (`IoError` / `InvalidData`) are also absorbed — the model keeps defaults. Corruption is therefore indistinguishable from never-saved **only at the model load boundary**, not at the provider. Apps that need load-error UX (e.g. "data was reset because corrupt") must **extend the `IModel` contract deliberately** (return `PersistenceResult` from `loadState`, or a documented status signal) — do not "fix" this with ad-hoc parallel load-result APIs or model-layer duplicate persistence logging.
- The `PersistenceError` taxonomy distinctions are load-bearing at the provider and in `PersistenceResult`: `NotFound` = first-run; `IoError` vs `InvalidData` vs `CommitError` are operational failures with distinct meanings — do not collapse them into a generic failure.
- **`PersistenceResult` is a C++20 `std::expected` analogue** (project pins C++20). On C++23, consider migrating both specializations to `std::expected` in a dedicated change; do not remove the type while on C++20.
- AppDataLocation precondition: feature-state storage paths use `QStandardPaths::AppDataLocation`. Tests enable Qt test mode (`QStandardPaths::setTestModeEnabled(true)` in `tests/main.cpp`) so test runs never write the developer's real application data — new test targets inherit this contract.

#### Configuration channels

| Channel | What | Mechanism |
|---|---|---|
| `app.env` | Build-time metadata SSOT | `scripts/env.sh` → Conan → CMake → `apply_app_metadata()` compile defs |
| `QSettings(ORGANIZATION_NAME, APP_NAME)` | Window/UI chrome (geometry/state) | `AppMainWindow` `finishConstruction()` restore + `closeEvent` save. Both paths observe `QSettings::status()` and log a warning on non-`NoError` (never block close). |
| `IPersistenceProvider` | **Feature state only** — not window chrome | `FilePersistenceProvider` JSON via `QSaveFile`; test double `MemoryPersistenceProvider`. Chrome stays on QSettings; do not merge channels. |

Persistence keys use the `APP_ID` compile definition from `app.env` (e.g. `APP_ID ".CounterState"`).

### Threading

- All QObject-derived application objects are GUI-thread-affine.
- Do not access or mutate them from worker threads.
- Models execute on the GUI thread. Persistence operations are synchronous.
- EventSystem delivery is always queued (Qt::QueuedConnection).
- **Thread-guard asymmetry (intentional today):** EventSystem publish/subscribe are GUI-thread-only and **runtime-guarded in all builds** (wrong-thread calls log `qCCritical(appEvent)` and no-op); debug builds also `Q_ASSERT`. Models and `FilePersistenceProvider` use **debug-only** `Q_ASSERT` GUI-thread checks (no runtime guard). That asymmetry matches the Async Policy: no worker threads exist yet; runtime guards for models/providers are deferred until a real workload requires them.
- The EventSystem's BusRegistry mutex protects dispatcher creation, not event delivery.
- The EventSystem is not a general cross-thread synchronization mechanism.
- Logging is thread-safe (QMutex in message handler).
- No API implies general thread safety merely because it contains a mutex.

### Async Policy

The template currently has **no background-worker architecture**. Persistence is intentionally synchronous on the GUI thread. Do not introduce `QThread`, `QtConcurrent`, or other asynchronous machinery merely to avoid synchronous work without first establishing the workload and ownership/threading requirements.

If actual long-running work appears, follow this positive escalation path:

1. Establish measured workload (evidence, not fear of sync I/O)
2. Identify the operation boundary
3. Define worker ownership
4. Define result delivery
5. Define cancellation
6. Define shutdown semantics
7. Only then introduce `QThread`/`QtConcurrent` (smallest appropriate mechanism)

Do not skip to step 7. A documented non-goal becomes a bug when the workload is real; the path above makes the exception deliberate.

### String Boundary Policy

- **Qt / UI / application-facing APIs:** `QString`
- **Generic event payloads:** `std::string` unless Qt-specific semantics are required

Example: `DemoLogEvent.message` is `std::string` at the bus boundary; the AppLog presenter converts once with `QString::fromStdString` when feeding the Qt model layer. Do not convert a single field across the boundary without establishing a repository-wide policy.

### Logging Categories

| Category | Use |
|---|---|
| `appMain` | Bootstrap / main |
| `appFeature` | Feature widgets and presenters |
| `appPersistence` | Model persistence load/save |
| `appEvent` | EventSystem delivery diagnostics (type mismatch, etc.) |
| `appService` | Service handlers (e.g. DemoConsoleLogService) |

Diagnostic logging uses `qC*` categories and the custom message handler — **not** the EventSystem.

### Feature Structure (MVP Convention)

Each feature lives in `src/features/{name}/` with these components:

```
src/features/myfeature/
├── myfeaturecommon.h → feature-local shared types (when needed)
├── model/        → data + persistence (implements IModel)
├── presenter/    → wiring between model and widget
└── widget/       → UI (no business logic)
```

- **`{name}common.h`** — always emitted by the generator. Use for cross-layer feature types (e.g. `LogDelta` in `applogcommon.h`). Reference features test via public API + `findChild` and do **not** declare `friend` test classes; the generator matches that philosophy.
- **Model** (`model/`) — owns application state, exposes getters/setters, handles persistence via `IPersistenceProvider`. Implements `IModel` (`loadState()`, `saveState() -> PersistenceResult<void>`). **This template's Model is application state (IModel), not `QAbstractItemModel`.** For Qt item views prefer `QAbstractListModel` (or another `QAbstractItemModel` subclass) in a dedicated model/view feature — do not force feature-state persistence through the item-model hierarchy.
- **Presenter** (`presenter/`) — receives model and widget via constructor **by reference** (`Model &`, `Widget &`; non-null encoded in the type; QPointer members still observe mid-session destruction), wires signals/slots between them
- **Widget** (`widget/`) — UI only, no business logic, concrete class. View API convention: **`display*`** methods for presenter→view state pushes (e.g. `CounterWidget::displayCounter`). Full-state replacement may use **`set*`** when the call promises replacement semantics (e.g. `AppLogWidget::setLogMessages` — clears then adds; the name is deliberate).

Widget UI slots follow Qt auto-connect naming: `on_<uiObjectName>_clicked` etc.
The slot name must match the `.ui` object name or the slot will never fire.

`AppMainWindow` constructs all three via `constructFeatures()` / `finishConstruction()` and wires them together.

### Adding a New Feature

Follow this wiring order. Items marked **silent failure** do not produce a compile error if omitted — verify them explicitly.

1. Generate the feature: `./scripts/generate.sh feature MyThing`
2. Inspect the generated files (model/presenter/widget + test stub)
3. Wire in `AppMainWindow::constructFeatures()` (the sole composition-wiring site; called from both ctors after the provider is bound) in this order: **model → widget → presenter** (presenter does **not** create model/widget; it takes model/view by reference). Provider must be bound before models (`IPersistenceProvider&` into the model).
4. **Register the model** in `constructFeatures()`: `registerModel(m_myThingModel)`. This is the sole `m_models` append site (non-null assert + list append). **Silent failure:** omitting `registerModel` disables shutdown persistence for that feature — there is no census assert to catch it. Models return `PersistenceResult<void>` from `saveState()`; do not invent a parallel save path.
5. Connect model→presenter signals and widget→presenter request signals (widget UI slots use `on_<uiObjectName>_...` auto-connect names that match the `.ui` object).
6. **Perform initial model→view synchronization** in the presenter ctor after connects (e.g. `m_view->displayX(m_model->x())`). Models load persisted state in their constructors *before* the presenter exists — emissions during model construction have no subscribers. **Silent failure:** without this step, a generated feature with restored state shows a stale/empty view until first interaction. See `CounterPresenter.cpp` / `AppLogPresenter.cpp`.
7. **Add the feature widget to the central layout** in `AppMainWindow::finishConstruction()` via `mainLayout->addWidget(...)`. The central layout is code-built — do not add feature widgets to `AppMainWindow.ui`. **Silent failure:** omitting this leaves the widget constructed but not shown.
8. Persistence behavior:
   - load in the model ctor (`loadState()`; treat `NotFound` as first-run; operational load errors keep defaults — see Persistence section)
   - shutdown save is driven by the composition-root `m_models` loop in `closeEvent` (models registered via `registerModel`)
9. Register cross-component events only where justified (EventSystem)
10. Add tests as flat files under `tests/features/` (e.g. `tests/features/MyThingTest.cpp`)
11. Verify shutdown persistence (closeEvent loop covers the new model via `registerModel` → `m_models`)
12. Build and run the complete test suite: `./scripts/build.sh --test ON`

`generate.sh` is the canonical starting point for new features. Generated output matches the reference conventions (`CounterModel` / `AppLogModel`): provider-by-reference, `PersistenceResult` forwarding in `saveState()`, and `NotFound` as first-run in `loadState()`. Generated `saveState()` **calls the provider** (it does not silently return success). Generated presenter scaffolds a **by-reference** ctor and initial model→view sync (comment + example; fill in real calls). Generated tests include provider-injection + save-failure forwarding checks **and** behavioral scaffolds (`RestoredStateVisibleWithoutInteraction`, `LoadStateRestoresPersistedValues`) — fill those in once `loadState()` and the view API exist. `loadState()` body remains feature-specific/TODO. Do not "correct" generated code into a different architecture.

Composition-root extension summary: add triples in `constructFeatures()`, call `registerModel` for each model, add widgets with `mainLayout->addWidget(...)` in `finishConstruction()`. Do not add feature widgets to `AppMainWindow.ui`. There is no model-census assert to update.

### Service Registration

**Scope:** `services::*` retains **free-function EventSystem subscriptions only**. It is not a general DI container. Qt-object services use QObject parent-child ownership; do not expand the registry mechanism to wire arbitrary services (see `ServiceRegistry.hpp`).

Services are registered explicitly in `services::registerAll()` in `src/services/registry/ServiceRegistry.cpp`.

**Registration lifecycle** (called from `main()`, not from AppMainWindow):

```text
QApplication exists
    ↓
services::registerAll()
    ↓
application runs
    ↓
services::unregisterAll()   // called from main() on every path after registration
    ↓
QApplication destruction
```

- `registerAll()` is process-level from `main()` after QApplication construction; there is no AppMainWindow wiring.
- `unregisterAll()` resets retained subscriptions; it is idempotent. `main()` calls it on the normal path, the smoke-test path, **and** the exception catch paths — every path after `registerAll()` unregisters before returning.
- Static file-scope `Subscription` storage remains the storage pattern for free-function handlers, but static storage is **not** the lifecycle mechanism — explicit `unregisterAll()` is.

To add a new service:
1. Create the service handler function (must match `void(const EventType&)`)
2. In `services::registerAll()`, **store** the `events::Subscription` returned by free-function subscribe
3. Reset the retained subscription in `services::unregisterAll()`
4. Include the necessary headers

Example:
```cpp
namespace {
events::Subscription g_myServiceSubscription;
}

void registerAll() {
    g_myServiceSubscription =
        events::subscribe<MyEvent>(MyService::handle);
}

void unregisterAll() {
    g_myServiceSubscription.reset();
}
```

The free-function `subscribe` overload is `[[nodiscard]]`. Discarding the Subscription disconnects immediately — that is a bug, not an optional style.

There is no auto-registration macro. Registration is visible and explicit.

### Lifecycle

- AppMainWindow is the composition root.
- Shutdown is synchronous and deterministic.
- `closeEvent` saves QSettings chrome (observes `QSettings::status()`, logs a warning on non-`NoError`) + iterates `QList<IModel*>` calling `saveState()`; results are observed (logged via `qCDebug(appPersistence)` when error) but never block shutdown — no `processEvents()` pumping.
- Service teardown is explicit: `services::unregisterAll()` runs from `main()` on every path after `registerAll()` (normal, smoke-test, and exception catch paths) before returning — and thus before `QApplication` destruction on those paths.
- QObject destruction follows the ownership tree.
- Model destructors must not call the persistence provider (locked by `LifecycleTest.ModelDestructorDoesNotTouchProvider` via a tracking double).

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
CI steps: architecture-boundary check (Linux), **Compiler profile guard (Linux)** (runner compiler major must match `conan/profiles/linux`), **Conan profile pin check** (all platforms; non-fatal while the linux pin targets incoming ubuntu-26.04 — via `scripts/check-conan-profile-toolchain.sh`), `scripts/build.sh --test ON` (includes production `--smoke-test`; missing smoke binary hard-fails under CI), generator smoke test (`scripts/test-generator.sh`), then platform packaging via `.github/scripts/`.
See `.github/workflows/ci.yml`.

---

*This documentation describes the intended architecture and extension conventions. For load-bearing implementation details, verify against the code when the documentation and implementation disagree.*
