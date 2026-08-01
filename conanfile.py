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
    # pulls in no graphics, input or sound framework at all.
    # input_backend defaults to "auto", meaning "whatever gfx_backend is",
    # so asking for sdl3 windows gets sdl3 input for free.
    #
    # sound_backend deliberately does NOT follow, and defaults to "null".
    # Input follows graphics because a window without input is useless.
    # Sound is orthogonal, and "auto" would mean every existing
    # -o gfx_backend=sdl3 build silently began opening an audio device.
    #
    # raylib is absent from sound_backend's values because it does not
    # implement that seam, and an unlisted value is the cheapest possible
    # way to say so: Conan refuses it before anything is downloaded.
    options = {
        "gfx_backend": ["null", "sdl3", "raylib"],
        "input_backend": ["auto", "null", "sdl3", "raylib"],
        "sound_backend": ["null", "sdl3"],
        "network_backend": ["null", "sockets"],
    }

    default_options = {
        "gfx_backend": "null",
        "input_backend": "auto",
        "sound_backend": "null",
        "network_backend": "null",
    }

    @property
    def _input_backend(self):
        # "auto" is resolved here rather than by rewriting the option,
        # which Conan does not allow a recipe to do to its own options.
        if self.options.input_backend == "auto":
            return str(self.options.gfx_backend)

        return str(self.options.input_backend)

    @property
    def _selected_frameworks(self):
        # The distinct real frameworks this configuration would link.
        # "null" is not one: it is how a subsystem opts out.
        #
        # Neither is "sockets", and that is the one exception rather
        # than a second rule.  The two reasons this set exists are a
        # process-global event queue and a doubled dependency graph, and
        # a backend naming the operating system's own socket API has
        # neither: it adds no package, no lockfile entry and no queue
        # anything else could compete for.
        backends = {
            str(self.options.gfx_backend),
            self._input_backend,
            str(self.options.sound_backend),
            str(self.options.network_backend),
        }
        backends.discard("null")
        backends.discard("sockets")

        return backends

    def validate(self):
        # Graphics and input would fight over one OS event queue, and
        # naming a second framework anywhere links two of them into one
        # process.  CMake refuses this too; catching it here keeps a
        # build from downloading a framework it cannot use.
        if len(self._selected_frameworks) > 1:
            raise ConanInvalidConfiguration(
                "gfx_backend, input_backend, sound_backend and "
                "network_backend name more than one framework, which "
                "would compete for one event queue and double the "
                "dependency graph"
            )

    def requirements(self):
        self.requires("nlohmann_json/3.12.0", override=True)
        self.requires("json-schema-validator/2.4.0")

        # Unconditional, unlike the backends below.
        # antwika::gfx decodes an image once for every backend to
        # upload, so the decoder is not part of anyone's graph in
        # particular.
        self.requires("stb/cci.20220909")

        # Also unconditional, and for the same reason: antwika::gfx's
        # 3D math types are built on GLM whichever backend draws them.
        # Header-only, so it costs a link line nothing.
        self.requires("glm/1.0.1")

        # Any option selecting a framework is enough to need it.
        backends = self._selected_frameworks

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
        toolchain.cache_variables["ANTWIKA_SOUND_BACKEND"] = str(
            self.options.sound_backend
        )
        toolchain.cache_variables["ANTWIKA_NETWORK_BACKEND"] = str(
            self.options.network_backend
        )
        toolchain.generate()

        CMakeDeps(self).generate()
