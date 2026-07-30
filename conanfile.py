from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeDeps, CMakeToolchain

class AntwikaConan(ConanFile):
    settings = (
        "os",
        "compiler",
        "build_type",
        "arch",
    )

    # Which backends under backends/ to compile and link.
    # Only a selected one contributes a dependency, so the default build
    # pulls in no graphics or input framework at all.
    # input_backend defaults to "auto", meaning "whatever gfx_backend is",
    # so asking for sdl3 windows gets sdl3 input for free.
    options = {
        "gfx_backend": ["null", "sdl3", "raylib"],
        "input_backend": ["auto", "null", "sdl3", "raylib"],
    }

    default_options = {
        "gfx_backend": "null",
        "input_backend": "auto",
    }

    @property
    def _input_backend(self):
        # "auto" is resolved here rather than by rewriting the option,
        # which Conan does not allow a recipe to do to its own options.
        if self.options.input_backend == "auto":
            return str(self.options.gfx_backend)

        return str(self.options.input_backend)

    def validate(self):
        # Two real frameworks would fight over one OS event queue.
        # CMake refuses this too; catching it here keeps a build from
        # downloading a framework it is about to be told it cannot use.
        backends = {str(self.options.gfx_backend), self._input_backend}
        backends.discard("null")

        if len(backends) > 1:
            raise ConanInvalidConfiguration(
                "gfx_backend and input_backend name two different "
                "frameworks, which would compete for one event queue"
            )

    def requirements(self):
        self.requires("nlohmann_json/3.12.0", override=True)
        self.requires("json-schema-validator/2.4.0")

        # Either option selecting a framework is enough to need it.
        backends = {str(self.options.gfx_backend), self._input_backend}

        if "sdl3" in backends:
            self.requires("sdl/3.2.20")
        elif "raylib" in backends:
            self.requires("raylib/6.0")

    def build_requirements(self):
        self.test_requires("gtest/1.17.0")

    def generate(self):
        # Hand the backend choices to CMake, so one -o flag drives both.
        toolchain = CMakeToolchain(self)
        toolchain.cache_variables["ANTWIKA_GFX_BACKEND"] = str(
            self.options.gfx_backend
        )
        toolchain.cache_variables["ANTWIKA_INPUT_BACKEND"] = (
            self._input_backend
        )
        toolchain.generate()

        CMakeDeps(self).generate()
