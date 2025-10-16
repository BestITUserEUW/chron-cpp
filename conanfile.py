from conan import ConanFile
from conan.tools.files import get, copy, rmdir
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.env import VirtualBuildEnv
from conan.tools.build import check_min_cppstd

import os

required_conan_version = ">=2.18.1"


class ChronCppConan(ConanFile):
    name = "cron-cpp"
    description = "cron-cpp is a C++20 scheduling library using cron formatting"
    license = "UNLICENSE"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/BestITUserEUW/chron-cpp"
    topics = "chron"
    generators = "cmake"
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"

    options = {
        "shared": [True, False],
    }

    default_options = {
        "shared": False,
    }

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.23 <4]")

    def validate(self):
        if self.settings.get_safe("compiler.cppstd"):
            check_min_cppstd(self, self._min_cppstd)

    def layout(self):
        cmake_layout(self, src_folder=".")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["ORYX_CHRON_BUILD_SHARED_LIBS"] = self.options.shared
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(
            self,
            pattern="LICENSE",
            dst=os.path.join(self.package_folder, "licenses"),
            src=self.source_folder,
        )
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.libs = ["chron-cpp"]

    @property
    def _min_cppstd(self):
        return 20

    @property
    def _compilers_minimum_version(self):
        return {
            "Visual Studio": "17",
            "msvc": "1938",
            "gcc": "13",
            "clang": "16",
            "apple-clang": "16",
        }
