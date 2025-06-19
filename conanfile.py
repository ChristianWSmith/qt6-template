from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake
from conan.tools.microsoft import VCVars
import os
from pathlib import Path
from conan.tools.files import copy


class MyConanApp(ConanFile):
    name = os.environ.get('APP_NAME', 'DEFAULT_APP_NAME')
    version = os.environ.get('APP_VERSION', 'DEFAULT_APP_VERSION')
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "VCVars", "CMakeToolchain"
    requires = [
        "fmt/[>=11.2.0 <12]",
        "vulkan-loader/[>=1.4.309.0 <2]",
        "vulkan-headers/[>=1.4.309.0 <2]",
        "cxxopts/[>=3.3.1 <4]",
        "gtest/[>=1.16.0 <2]",
        "zlib/[>=1.3.1 <2]",
    ]
    default_options = {
        "zlib/*:shared": True
    }


    def layout(self):
        self.folders.build = os.environ.get("BUILD_DIR", "build")

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
            "CMAKE_PREFIX_PATH": os.environ.get("QT_CMAKE_DIR", "./Qt"),
            "APP_NAME": os.environ.get("APP_NAME", "MyConanApp"),
            "APP_DESCRIPTION": os.environ.get("APP_DESCRIPTION", "My Conan App Description"),
            "APP_VERSION": os.environ.get("APP_VERSION", "0.1.0"),
            "APP_ID": os.environ.get("APP_ID", "com.example.MyConanApp"),
            "QT_DEBUG_FIND_PACKAGE": "ON",
            "BUILD_TESTING": os.environ.get("BUILD_TESTING", "OFF"),
            "UT_NAME": os.environ.get("UT_NAME", "UnitTests"),
        })
        cmake.build()
        if os.getenv("UPDATE_TRANSLATIONS", "OFF") == "ON":
            cmake.build(target="update_translations")
        self.copy_shared_libs()

    def copy_shared_libs(self):
        out_dir = self.build_folder
        if self.settings.os == "Windows":
            exts = ["*.dll"]
            out_dir = os.path.join(self.build_folder, self.settings.build_type)
        elif self.settings.os == "Linux":
            exts = ["*.so", "*.so.*"]
        elif self.settings.os == "Macos":
            exts = ["*.dylib"]
        else:
            return

        for dep in self.dependencies.values():
            for bindir in dep.cpp_info.bindirs:
                for pattern in exts:
                    copy(self, pattern, bindir, out_dir)
