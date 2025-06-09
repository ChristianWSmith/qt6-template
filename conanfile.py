from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake
import os

class MyAppConan(ConanFile):
    name = os.environ['APP_NAME']
    version = os.environ['APP_VERSION']
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"
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
