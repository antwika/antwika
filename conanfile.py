from conan import ConanFile

class AntwikaConan(ConanFile):
    settings = (
        "os",
        "compiler",
        "build_type",
        "arch",
    )

    generators = (
        "CMakeToolchain",
        "CMakeDeps",
    )

    def build_requirements(self):
        self.test_requires("gtest/1.17.0")
