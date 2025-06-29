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

This script builds the application using Conan and CMake.

- **Default build type**: `Release`
- **Default options**: `--test ON`, `--update-translations ON`, `--clean OFF`
- **App output**: `${BUILD_DIR}/${APP_NAME}` (e.g. `build/MyApp`)
- **Qt installation**: Automatically installs Qt if it's not already available
- **CMake re-glob workaround**: Touches `CMakeLists.txt` to ensure newly added source files are picked up

#### Example

```bash
./scripts/build.sh --build-type Debug --clean ON
```

This performs a clean debug build with tests and translations enabled.

### 4. Run the App

```bash
./run.sh [--build-type Debug|Release] [--clean ON|OFF] [--rebuild ON|OFF] [--test ON|OFF] [--update-translations ON|OFF] [-- <app args>]
```

This will optionally build and then run the application.

- **Default build type**: `Debug`
- **Default options**: `--clean OFF`, `--rebuild OFF`, `--test OFF`, `--update-translations OFF`
- **App output**: `${BUILD_DIR}/${APP_NAME}` (e.g. `build/MyApp`)
- **Qt installation**: Automatically installs Qt if not found
- **Platform-aware**: Handles `.exe` on Windows and `.app` bundles on macOS
- **CMake re-glob workaround**: Touches `CMakeLists.txt` during build phase

#### Example

```bash
./run.sh --build-type Release --rebuild ON --windowed --lang en
```

This performs a Release build if needed and passes `--windowed --lang en` as arguments to the running app (not implemented here).

## 📁 Project Structure

```
.
├── scripts/
├──── build.sh            # Builds the project (calls Conan & CMake)
├──── run.sh              # Launches built binary
├──── env.sh              # Sourced environment (loads app.env)
├──── clean.sh            # Used to clean build/Qt dir(s)  
├── src/
├──── *.cpp / *.h / *.ui  # Your application code
├── resources/
├──── resources.qrc       # Qt resources (for baked-in files)
├──── icons/              # Icons for the application
├──── ts/                 # Qt translation files
├── app.env               # App name, ID, version, Qt (used by all tools)
├── CMakeLists.txt        # CMake build script
├── conanfile.py          # Conan recipe
├── Pipfile               # Used to manage conan / aqt
├── conan/                # Conan profiles
└── .github/              # GitHub Actions workflows
```

## Architecture & Feature Design

🚨 **THIS IS COMPLETELY OPTIONAL AND CAN BE IGNORED/REMOVED PER DEVELOPER PREFERENCE** 🚨

This project follows a **strictly modular feature-first architecture**. Each feature exists as a self-contained unit under the `features/` directory, with a standard structure:

- `model/`: The data/state layer (e.g. `FooModel`, `IFooModel`)
- `presenter/`: The logic and orchestration layer (e.g. `FooPresenter`, `IFooPresenter`)
- `widget/`: The view/UI layer (e.g. `FooWidget`, `IFooWidget`)
- Tests are mirrored under `tests/features/` using the same hierarchy.

All communication between features must happen **exclusively via the centralized event system** (see `src/events/`), not via direct references. This ensures loose coupling and high testability.

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


## 🧪 Developer Workflows

### Add a New Dependency

Edit `conanfile.py`:

```python
requires = [
    "fmt/[>=10.2.1]",
    "spdlog/[>=1.12]",
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

The project uses flexible version constraints (e.g., `fmt/[>=11.2.0 <12]`) to always resolve the latest compatible version across platforms. During the lockfile generation process, Conan creates a separate lockfile per platform and attempts to merge them. If this merge fails, it indicates that no single version satisfies all platforms in the given version range. In such cases, you’ll need to determine a version that works universally and explicitly pin it in your `conanfile.py`. This is unlikely, however.

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

Each PR or push triggers a cross-platform build.  For Windows, the artifact will be an application installer as well as a portable version.  For Linux, you'll get an AppImage and a tarball.  For macOS, you'll get a .app folder.

## ✨ UI Development

- Edit `.ui` files using **Qt Designer**
- Edit `.ts` files in **Qt Linguist**.  You can always update these using `lupdate`/`lrelease`, or you can just have the included `scripts/build.sh` handle it automatically.
- Add images or resources to `resources.qrc`

## 🐳 Dev Container

This project includes a robust and opinionated **dev container** setup designed for maximum parity between local development and CI/CD.

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

- Use `pipenv run` for consistent tool execution.  All included helper scripts do this automatically as needed.
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
