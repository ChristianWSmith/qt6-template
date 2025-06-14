from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake
from conan.tools.microsoft import VCVars
import os


class MyConanApp(ConanFile):
    name = os.environ.get('APP_NAME', 'DEFAULT_APP_NAME')
    version = os.environ.get('APP_VERSION', 'DEFAULT_APP_VERSION')
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "VCVars", "CMakeToolchain"
    requires = [
        "fmt/[>=0]",
        "vulkan-loader/[>=0]",
        "vulkan-headers/[>=0]"
    ]

    def layout(self):
        self.folders.build = os.environ.get("BUILD_DIR", "build")

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
            "CMAKE_PREFIX_PATH": os.environ.get("QT_CMAKE_DIR", "./Qt"),
            "APP_NAME": os.environ.get("APP_NAME", "MyConanApp"),
            "APP_VERSION": os.environ.get("APP_VERSION", "0.1.0"),
            "APP_ID": os.environ.get("APP_ID", "com.example.MyConanApp"),
            "QT_DEBUG_FIND_PACKAGE": "ON"
        })
        cmake.build()
