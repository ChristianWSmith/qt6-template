from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake

class MyAppConan(ConanFile):
    name = "myapp"
    version = "0.1"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"
    requires = "fmt/10.2.1"

    def layout(self):
        self.folders.build = "build"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
