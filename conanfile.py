from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake
from conan.tools.microsoft import VCVars
import os


class MyAppConan(ConanFile):
    name = os.environ.get('APP_NAME', 'DEFAULT_APP_NAME')
    version = os.environ.get('APP_VERSION', 'DEFAULT_APP_VERSION')
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "VCVars", "CMakeToolchain"
    requires = [
        "fmt/10.2.1",
        "vulkan-loader/1.3.268.0",
        "vulkan-headers/1.3.268.0"
    ]

    def layout(self):
        self.folders.build = "build"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
