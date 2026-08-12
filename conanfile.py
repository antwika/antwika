from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMakeDeps, CMakeToolchain

class AntwikaConan(ConanFile):
    license = "Apache-2.0"

    settings = (
        "os",
        "compiler",
        "build_type",
        "arch",
    )

    options = {
        "gfx_backend": ["null", "raylib"],
        "input_backend": ["auto", "null", "raylib"],
        "sound_backend": ["null", "raylib"],
        "network_backend": ["null", "sockets"],
    }

    default_options = {
        "gfx_backend": "null",
        "input_backend": "auto",
        "sound_backend": "null",
        "network_backend": "null",
    }

    @property
    def _input_backend(self) -> str:
        if self.options.input_backend == "auto":
            return str(self.options.gfx_backend)

        return str(self.options.input_backend)

    @property
    def _selected_frameworks(self) -> set[str]:
        backends = {
            str(self.options.gfx_backend),
            self._input_backend,
            str(self.options.sound_backend),
            str(self.options.network_backend),
        }
        backends.discard("null")
        backends.discard("sockets")

        return backends

    def validate(self) -> None:

        if len(self._selected_frameworks) > 1:
            raise ConanInvalidConfiguration(
                "gfx_backend, input_backend, sound_backend and "
                "network_backend name more than one framework, which "
                "would compete for one event queue and double the "
                "dependency graph"
            )

    def requirements(self) -> None:
        self.requires("nlohmann_json/3.12.0", override=True)
        self.requires("json-schema-validator/2.4.0")

        self.requires("stb/cci.20240531")

        self.requires("glm/1.0.1")

        backends = self._selected_frameworks

        if "raylib" in backends:
            self.requires("raylib/6.0")

    def build_requirements(self) -> None:
        self.test_requires("gtest/1.18.0")

    def generate(self) -> None:
        toolchain = CMakeToolchain(self)
        toolchain.cache_variables["ANTWIKA_GFX_BACKEND"] = str(
            self.options.gfx_backend
        )
        toolchain.cache_variables["ANTWIKA_INPUT_BACKEND"] = (
            self._input_backend
        )
        toolchain.cache_variables["ANTWIKA_SOUND_BACKEND"] = str(
            self.options.sound_backend
        )
        toolchain.cache_variables["ANTWIKA_NETWORK_BACKEND"] = str(
            self.options.network_backend
        )
        toolchain.generate()

        CMakeDeps(self).generate()
