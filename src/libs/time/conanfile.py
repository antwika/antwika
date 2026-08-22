from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout


class AntwikaTimeConan(ConanFile):
    name = "antwika-time"
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

    def layout(self) -> None:
        self.folders.build_folder_vars = ["options.coverage"]
        cmake_layout(self)

    def generate(self) -> None:
        toolchain = CMakeToolchain(self)
        toolchain.cache_variables["ENABLE_COVERAGE"] = bool(
            self.options.coverage
        )
        toolchain.generate()

    def build(self) -> None:
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self) -> None:
        cmake = CMake(self)
        cmake.install()

    def package_info(self) -> None:
        self.cpp_info.libs = ["antwika_time"]
        self.cpp_info.set_property("cmake_file_name", "antwika-time")
        self.cpp_info.set_property("cmake_target_name", "antwika::time")
