from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake
from conan.tools.microsoft import VCVars
import os

class MyAppConan(ConanFile):
    name = os.environ['APP_NAME']
    version = os.environ['APP_VERSION']
    settings = "os", "arch", "compiler", "build_type"
    requires = [
        "fmt/10.2.1",
        "vulkan-loader/1.3.268.0",
        "vulkan-headers/1.3.268.0"
    ]

    def layout(self):
        self.folders.build = "build"

    def generate(self):
        cmake = CMakeDeps(self)
        cmake.generate()

        if os.environ['PLATFORM'] == "windows":
            ms = VCVars(self)
            ms.generate()

        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
