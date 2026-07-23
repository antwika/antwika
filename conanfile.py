from conan import ConanFile

class antwikaConan(ConanFile):
    name = "antwika"
    version = "0.1"
    package_type = "application"

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

    def requirements(self):
        self.requires("gtest/1.17.0")
