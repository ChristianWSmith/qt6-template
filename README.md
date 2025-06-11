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

---

## 🧰 Getting Started

### 0. Create Repo

See that green `Use this template ▼` button in the upper right corner of this page?  Click that and hit `Create a new repository`.

### 1. Install Requirements

- Bash (duh)
- Python
- Pipenv
- CMake
- C++ compiler (MSVC, Clang, or GCC)
- `clangd` and `clang-tidy` if using vscode integrations

### 1.5. Configure VSCode
```bash
./scripts/configure-vscode.sh
```
This will set you up with a `.vscode` directory with some sensible default settings.  Make sure you rerun this command if you ever update your app name in `app.env`.  Or you can modify the contents of the `.vscode` directory manually.

### 2. Install Qt Version

```bash
./scripts/install_qt.sh
```
This will install the version of Qt specified in `app.env`.  It'll be installed to the `Qt/` directory, which is gitignored.

### 2. Build the Project

```bash
./scripts/build.sh [Debug|Release]
```
This will build the application and put it in `${BUILD_DIR}/${APP_NAME}` (`build/MyApp` by default)

Defaults to `Release` if unspecified.  Installs Qt if not found.

### 4. Run the App

```bash
./run.sh [Debug|Release]
```
This will build and run the application.

Defaults to `Debug` if unspecified.  Installs Qt if not found.

---

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

---

## 🧪 Developer Workflows

### Add a New Dependency

Edit `conanfile.py`:

```python
requires = [
    "fmt/[>=10.2.1]",
    "spdlog/[>=1.12]",
]
```

Then re-run:

```bash
./scripts/build.sh
```

Then re-lock as shown below.

### Update Dependency Lock

```bash
./scripts/conan-lock-update.sh
```

The project uses flexible version constraints (e.g., `fmt/[>=0]`) to always resolve the latest compatible version across platforms. During the lockfile generation process, Conan creates a separate lockfile per platform and attempts to merge them. If this merge fails, it indicates that no single version satisfies all platforms in the given version range. In such cases, you’ll need to determine a version that works universally and explicitly pin it in your `conanfile.py`. This is unlikely, however.

### Change App Name / ID / Version

Edit `app.env`:

```env
APP_NAME=MyApp
APP_ID=com.example.MyApp
APP_VERSION=0.1.0
```

These values will flow automatically into:

- Conan package metadata
- CMake definitions
- C++ macros (e.g. `APP_NAME`, `APP_VERSION`)

---

## ⚙️ CI/CD: GitHub Actions

### Build Matrix

- **Branches `main/develop` →** Release builds

Each PR or push triggers a cross-platform build.  For Windows, the artifact will be an application installer.  For Linux, you'll get an AppImage.  For macOS, you'll get a .app folder.

---

## ✨ UI Development

- Edit `.ui` files using **Qt Designer**
- Add images or resources to `resources.qrc`
- Use `lupdate` and `lrelease` for translations (`*.ts` files)

---

## 💡 Tips

- Use `pipenv run` for consistent tool execution.
- The `env.sh` and `app.env` combo ensures your config stays centralized and clean.
- Want to rename the app? Just update `app.env`. No renaming needed elsewhere.
- `app_icon.ico` is used for Windows, `app_icon.png` is used for Linux, `app_icon.icns` is for macOS, and `app_icon.svg` is not used, but is included for posterity.

---

## 📝 License

MIT License. See [LICENSE](./LICENSE) for details.

---

## 📣 Contributing

This template is intended to kickstart Qt projects. If you have suggestions, feel free to open a PR or issue.
