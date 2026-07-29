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

    def requirements(self):
        self.requires("nlohmann_json/3.12.0", override=True)
        self.requires("json-schema-validator/2.4.0")

    def build_requirements(self):
        self.test_requires("gtest/1.17.0")
