from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class AntwikaLogConan(ConanFile):
    name = "antwika-log"
    version = "1.0.0"
    package_type = "static-library"

    settings = (
        "os",
        "compiler",
        "build_type",
        "arch",
    )

    options = {
        "fPIC": [True, False],
        "coverage": [True, False],
    }

    default_options = {
        "fPIC": True,
        "coverage": False,
    }

    exports_sources = (
        "CMakeLists.txt",
        "include/*",
        "src/*",
    )

    def config_options(self) -> None:
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self) -> None:
        self.requires("antwika-time/1.0.0", transitive_headers=True)

    def layout(self) -> None:
        self.folders.build_folder_vars = ["options.coverage"]
        cmake_layout(self)

    def generate(self) -> None:
        toolchain = CMakeToolchain(self)
        toolchain.cache_variables["ENABLE_COVERAGE"] = bool(
            self.options.coverage
        )
        toolchain.generate()

        CMakeDeps(self).generate()

    def build(self) -> None:
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self) -> None:
        cmake = CMake(self)
        cmake.install()

    def package_info(self) -> None:
        self.cpp_info.libs = ["antwika_log"]
        self.cpp_info.set_property("cmake_file_name", "antwika-log")
        self.cpp_info.set_property("cmake_target_name", "antwika::log")
