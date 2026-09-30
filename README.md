# qt6-template (Qt + CMake + Conan Template)

This is a modern, cross-platform C++ Qt application template using CMake, Conan, and GitHub Actions. It includes everything you need to hit the ground running with GUI development, dependency management, and CI/CD.

## 🚀 Features

- Qt GUI with CMake build system
- Conan package manager with lockfile support
- Pipenv-based Python tooling environment
- GitHub Actions CI for Linux, Windows, and macOS
- Debug/Release builds with proper toolchain handling
- Modular, maintainable layout
- Single source of truth for app name, ID, and version (`app.env`)
- Dev container for maximum dev/CI parity

## 🧰 Getting Started

### 0. Create Repo

See that green `Use this template ▼` button in the upper right corner of this page?  Click that and hit `Create a new repository` 😉

### 1. Install Requirements

- Bash (duh)
- Python
- Pipenv
- CMake
- Ninja
- C++ compiler (MSVC, Clang, or GCC)
- `clangd` and `clang-tidy` if using vscode integrations

OR

- Docker 🐋

### 1.5. Configure VSCode
```bash
./scripts/configure-vscode.sh
```
This will set you up with a `.vscode` directory with some sensible default settings.  Make sure you rerun this command if you ever update your app name in `app.env`.  Or you can modify the contents of the `.vscode` directory manually.

### 2. Install Qt Version

```bash
./scripts/install-qt.sh
```
This will install the version of Qt specified in `app.env`.  It'll be installed to the `Qt/` directory, which is gitignored.

### 3. Build the Project

```bash
./scripts/build.sh [--build-type Debug|Release] [--clean ON|OFF] [--test ON|OFF] [--update-translations ON|OFF]
```

This script builds the application using Conan and CMake. With `--test ON` (default), it configures and builds **and then executes the test suite via CTest**.

- **Default build type**: `Release`
- **Default options**: `--test ON`, `--update-translations ON`, `--clean OFF`
- **`--test ON`**: Builds the test suite **and runs** it via CTest after configure/build (`ctest --test-dir` runs outside pipenv)
- **Test binary**: single hardcoded target `UnitTests` (no `UT_NAME` indirection); tests link `Qt6::Test`
- **App output**: `${BUILD_DIR}/${APP_NAME}` (e.g. `build/MyApp`)
- **Qt installation**: Automatically installs Qt if it's not already available
- **Source discovery**: `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`; touch `CMakeLists.txt` manually only if a generator ever misses a new file (fallback, not the default path)

#### Example

```bash
./scripts/build.sh --build-type Debug --clean ON
```

This performs a clean debug build. With the default `--test ON`, the suite is also built and executed via CTest.

### 4. Run the App

```bash
./scripts/run.sh [--build-type Debug|Release] [--clean ON|OFF] [--rebuild ON|OFF] [--test ON|OFF] [--update-translations ON|OFF] [-- <app args>]
```

This will optionally build and then run the application.

- **Default build type**: `Debug`
- **Default options**: `--clean OFF`, `--rebuild OFF`, `--test OFF`, `--update-translations OFF`
- **App output**: `${BUILD_DIR}/${APP_NAME}` (e.g. `build/MyApp`)
- **Qt installation**: Automatically installs Qt if not found
- **Platform-aware**: Handles `.exe` on Windows and `.app` bundles on macOS
- **Source discovery**: Same `CONFIGURE_DEPENDS` glob as `build.sh`

#### Example

```bash
./run.sh --build-type Release --rebuild ON --windowed --lang en
```

This performs a Release build if needed and passes `--windowed --lang en` as arguments to the running app (not implemented here).

## 📁 Project Structure

```
.
├── scripts/                  # Developer façade (always prefer these over raw conan/cmake)
├──── build.sh                # Conan build + optional ctest (default --test ON)
├──── run.sh                  # Debug-run (builds if needed)
├──── env.sh                  # Sourced environment (loads app.env)
├──── clean.sh                # Used to clean build/Qt dir(s)
├──── generate.sh             # Feature/widget scaffolding
├──── install-qt.sh           # aqtinstall → local Qt/ (gitignored)
├── src/
├──── main.cpp                # Bootstrap; compiled into the APP_NAME executable (not _lib)
├──── appmainwindow/          # Composition root (AppMainWindow)
├──── core/                   # IModel, IPersistenceProvider (+ PersistenceResult)
├──── events/                 # EventSystem + domain event types
├──── features/<name>/        # {model,presenter,widget}/ per feature
├──── logging/                # qC* categories + message handler
├──── platform/               # {core/,theme/} FilePersistenceProvider, theme.hpp
├──── services/               # ConsoleLogService + ServiceRegistry
├──── widgets/<name>/         # Standalone widgets (e.g. ReusableWidget)
├── tests/
├──── features/               # Flat test files (e.g. CounterTest.cpp), not per-feature dirs
├──── MemoryPersistenceProvider.h
├──── main.cpp                # Custom gtest main (QApplication bootstrap)
├── resources/
├──── resources.qrc           # Qt resources (for baked-in files)
├──── icons/                  # Icons for the application
├──── i18n/                   # Qt translation files
├── .github/
├──── workflows/ci.yml        # Cross-platform build/test + bundle
├──── scripts/                # Platform packaging (AppImage, installers, .app)
├── app.env                   # App name, ID, version, Qt (used by all tools)
├── CMakeLists.txt            # CMake build script
├── conanfile.py              # Conan recipe (requires fmt/cxxopts; build_requires ninja; test_requires gtest)
├── conan/                    # Conan profiles
├── Pipfile                   # Used to manage conan / aqt
└── Dockerfile                # Toolchain-only container (Qt still via aqtinstall → Qt/)
```

## Architecture & Feature Design

The feature-first MV* layout is the template's intentional architecture. Implementation rules live in `AGENTS.md` (authoritative). This README is a getting-started guide.

This project follows a **strictly modular feature-first architecture**. Each feature exists as a self-contained unit under the `src/features/` directory, with a standard structure:

- `model/`: The data/state layer (e.g. `FooModel`). Implements `IModel`; `saveState()` returns `PersistenceResult<void>`.
- `presenter/`: The logic and orchestration layer (e.g. `FooPresenter`)
- `widget/`: The view/UI layer (e.g. `FooWidget`). UI slots follow Qt auto-connect naming (`on_<uiObjectName>_clicked` etc.); the slot name must match the `.ui` object name.
- Tests are flat files under `tests/features/` (e.g. `tests/features/CounterTest.cpp`), not per-feature directories.

Other `src/` modules: `core/` (IModel, IPersistenceProvider), `events/` (EventSystem), `services/` (registry + ConsoleLogService), `platform/` (FilePersistenceProvider, theme), `appmainwindow/` (composition root), `widgets/` (standalone widgets).

**Configuration channels:** `app.env` = build-time metadata SSOT; `QSettings(ORGANIZATION_NAME, APP_NAME)` = window/UI chrome (geometry/state); `IPersistenceProvider` = feature state (FilePersistenceProvider JSON via QSaveFile; MemoryPersistenceProvider test double). Persistence keys embed the `APP_ID` compile definition (e.g. `APP_ID ".CounterState"`).

**Shutdown persistence:** `AppMainWindow` keeps a `QList<IModel*>`; `closeEvent` loops every model's `saveState()`, observes results, and logs-and-continues (never blocks close on persistence failure). Atomic replace via `QSaveFile` is not a power-loss durability guarantee.

Intra-feature communication uses **Qt signals/slots**. Cross-component domain events use the centralized **EventSystem** (`src/events/`). Public EventSystem API is `events::publish` / `events::subscribe` only. Diagnostic logging uses `qC*` — never the event system. The `AppLog` feature demonstrates EventSystem subscription as an architectural demonstration (no production publisher). `AppLog` persistence demonstrates feature-state persistence, not a recommendation to persist production diagnostic logs.

See `AGENTS.md` for the full architectural contract.

To standardize and accelerate feature development, the repo includes a script:

### `scripts/generate.sh`

This script bootstraps all required source and test files for a new module. It supports two types:

- `feature`: Generates a full MV* scaffold (model, presenter, widget)
- `widget`: Generates only a standalone widget (no model/presenter/test)

It performs the following:

- Validates the name format (must be TitleCase and a valid C++ identifier)
- Creates the appropriate directory structure
- Generates `.h`, `.cpp`, and `.ui` files with placeholder logic
- Substitutes or removes generation markers to tailor files per type
- Applies `clang-format` to all generated files if available

Usage:

```bash
./scripts/generate.sh [feature|widget] Name
```

Examples:

```bash
./scripts/generate.sh feature MyThing
./scripts/generate.sh widget SimplePreview
```

Each invocation ensures the resulting module adheres to project architecture and coding conventions by default. Feature modules also include a `common.h` file for shared types and friend declarations, and a Google Test stub covering all layers.

Generated code must follow the same conventions as the reference features (`CounterModel` / `AppLogModel`) — e.g. models take a required `IPersistenceProvider&`. `AGENTS.md` is the authoritative contract for these conventions.


## 🧪 Developer Workflows

### Add a New Dependency

Edit `conanfile.py`:

```python
requires = [
    "fmt/[>=12.0.0 <13]",
    "cxxopts/[>=3.3.1 <4]",
]
```

Then re-lock as shown below.

### Update Dependency Lock

```bash
./scripts/conan-lock-update.sh
```

Then re-run:

```bash
./scripts/build.sh
```

The project uses flexible version constraints (e.g., `fmt/[>=12.0.0 <13]`) to always resolve the latest compatible version across platforms. During the lockfile generation process, Conan creates a separate lockfile per platform and attempts to merge them. If this merge fails, it indicates that no single version satisfies all platforms in the given version range. In such cases, you’ll need to determine a version that works universally and explicitly pin it in your `conanfile.py`. This is unlikely, however.

### Change App Name / ID / Version

Edit `app.env`:

```env
APP_NAME=MyApp
APP_ID=com.example.MyApp
APP_VERSION=0.1.0
APP_DESCRIPTION="My App Description"
APP_ORGANIZATION=MyOrganization
```

These values will flow automatically into:

- Conan package metadata
- CMake definitions
- C++ macros (e.g. `APP_NAME`, `APP_VERSION`)

---

## ⚙️ CI/CD: GitHub Actions

### Build Matrix

- **Branches `main/develop` →** Release builds

Each PR or push triggers a cross-platform build via `./scripts/build.sh --test ON`. CI also runs the architecture-boundary check (Linux) and the generator smoke test (`scripts/test-generator.sh`). For Windows, the artifact will be an application installer as well as a portable version. For Linux, you'll get an AppImage and a tarball. For macOS, you'll get a .app folder. Platform packaging lives in `.github/scripts/` (`linux-bundle-*.sh`, `mac-bundle-app.sh`, `windows-bundle.sh`); `CMakeLists.txt` `install()` covers only the executable target.

## ✨ UI Development

- Edit `.ui` files using **Qt Designer**
- Edit `.ts` files in **Qt Linguist**.  You can always update these using `lupdate`/`lrelease`, or you can just have the included `scripts/build.sh` handle it automatically.
- Add images or resources to `resources.qrc`

## 🐳 Dev Container

This project includes a robust and opinionated **dev container** setup designed for maximum parity between local development and CI/CD.

The Dockerfile is **toolchain-only** (compiler, cmake, pipenv, X11/build deps). It does not ship a parallel system Qt — Qt still comes from the local `Qt/` tree via `./scripts/install-qt.sh` (aqtinstall).

Highlights:

- 🧠 Intelligent `.bashrc` with with [starship](https://github.com/starship/starship)
- ⚡ Fast startup with persistent Python tooling
- 🛠️ Automatic virtualenv setup and activation inside the container
- 🖥️ Cross-platform GUI forwarding (may require XQuartz/VcXsrv setup for macOS/Windows)

To try it:

'''bash
./scripts/dev-container.sh
'''

## 🖼 Icon Generator

The `scripts/app-icon.sh` script creates a full suite of app icons from a single source SVG or PNG. It outputs all necessary platform-specific icon formats, including:

- `.ico` for Windows
- `.icns` for macOS
- `.png` for Linux
- Optional `.svg` copy for source tracking

This ensures:

- Platform-native icons in installers and app bundles
- Centralized source of truth (edit once, regenerate everywhere)
- Consistent visual identity across platforms

Usage:

'''bash
./scripts/app-icon.sh path/to/source_icon.svg
'''

Resulting icons are placed in `resources/icons/` and are automatically picked up by the build system.

## 💡 Tips

- Use `pipenv run` for consistent tool execution. Most included helper scripts do this automatically as needed. Exception: `build.sh --test ON` runs `ctest` directly after the Conan build.
- The `env.sh` and `app.env` combo ensures your config stays centralized and clean.
- Want to rename the app? Just update `app.env`. No renaming needed elsewhere.
- `app_icon.ico` is used for Windows, `app_icon.png` is used for Linux, `app_icon.icns` is for macOS, and `app_icon.svg` is not used, but is included for posterity.
- Delete/rename `.ts` files in `resources/i18n/` after updating the `APP_NAME` in `app.env`.
- You can modify which languages you want to bundle translations for in `CMakeLists.txt` here:
```cmake
qt_standard_project_setup(
    I18N_TRANSLATED_LANGUAGES en es de fr
)
```

---

## 📝 License

MIT License. See [LICENSE](./LICENSE) for details.

---

## 📣 Contributing

This template is intended to kickstart Qt projects. If you have suggestions, feel free to open a PR or issue.
