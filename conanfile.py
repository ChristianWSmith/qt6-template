from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake
import os
from conan.tools.files import copy


def _env_required(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise ConanInvalidConfiguration(
            f"{name} is not set. Configure the project through app.env/scripts "
            "(source scripts/env.sh, then build via ./scripts/build.sh)."
        )
    return value


class MyConanApp(ConanFile):
    name = _env_required('APP_NAME')
    version = _env_required('APP_VERSION')
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"
    requires = [
        "fmt/[>=12.0.0 <13]",
        "cxxopts/[>=3.3.1 <4]",
    ]
    build_requires = [
        "ninja/[*]",
    ]
    test_requires = [
        "gtest/[>=1.18.0 <2]",
    ]

    def layout(self):
        self.folders.build = os.environ.get("BUILD_DIR", "build")

    def build(self):
        # Qt root (not .../lib/cmake/Qt6): find_package searches <prefix>/lib/cmake.
        # Fail fast like APP_* — no silent "./Qt" default (E-017).
        qt_root = _env_required("QT_ROOT")
        cmake = CMake(self)
        cmake.configure(variables={
            "CMAKE_PREFIX_PATH": qt_root,
            "APP_NAME": _env_required("APP_NAME"),
            "APP_DESCRIPTION": _env_required("APP_DESCRIPTION"),
            "APP_VERSION": _env_required("APP_VERSION"),
            "ORGANIZATION_NAME": _env_required("ORGANIZATION_NAME"),
            "APP_ID": _env_required("APP_ID"),
            "I18N_TRANSLATED_LANGUAGES": _env_required("I18N_TRANSLATED_LANGUAGES"),
            "BUILD_TESTING": os.environ.get("BUILD_TESTING", "ON"),
        })
        cmake.build()
        if os.getenv("UPDATE_TRANSLATIONS", "OFF") == "ON":
            cmake.build(target="update_translations")
        self.copy_shared_libs()

    def copy_shared_libs(self):
        # Single-config Ninja (pinned in conan/profiles/{windows,darwin})
        # places the app binary at ${BUILD_DIR} root; multi-config generators
        # (Visual Studio) nest under ${BUILD_DIR}/${build_type}. Copy DLLs /
        # dylibs next to the binary under the Ninja layout. If a multi-config
        # generator is ever used on Windows, point out_dir at the build_type
        # subfolder instead (see scripts/lib/resolve-app-path.sh fallback).
        out_dir = self.build_folder
        if self.settings.os == "Windows":
            exts = ["*.dll"]
        elif self.settings.os == "Macos":
            exts = ["*.dylib"]
        else:
            return

        for dep in self.dependencies.values():
            for bindir in dep.cpp_info.bindirs:
                for pattern in exts:
                    copy(self, pattern, bindir, out_dir)
