from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain

class AntwikaConan(ConanFile):
    settings = (
        "os",
        "compiler",
        "build_type",
        "arch",
    )

    # Which graphics backend under backends/ to compile and link.
    # Only the selected one contributes a dependency, so the default build
    # pulls in no graphics framework at all.
    options = {
        "gfx_backend": ["null", "sdl3"],
    }

    default_options = {
        "gfx_backend": "null",
    }

    def requirements(self):
        self.requires("nlohmann_json/3.12.0", override=True)
        self.requires("json-schema-validator/2.4.0")

        if self.options.gfx_backend == "sdl3":
            self.requires("sdl/3.2.20")

    def build_requirements(self):
        self.test_requires("gtest/1.17.0")

    def generate(self):
        # Hand the backend choice to CMake, so one -o flag drives both.
        toolchain = CMakeToolchain(self)
        toolchain.cache_variables["ANTWIKA_GFX_BACKEND"] = str(
            self.options.gfx_backend
        )
        toolchain.generate()

        CMakeDeps(self).generate()
