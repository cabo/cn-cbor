import os
from conans import ConanFile, CMake, tools


class CnCborConan(ConanFile):
    name = "cn-cbor"
    version = "20200227"
    license = "BSD"
    url = "https://github.com/cose-wg/cn-cbor"
    description = """A constrained node implementation of CBOR in C"""
    topics = ("cn-cbor")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
    }
    default_options = {
        "shared": False,
    }
    generators = "cmake"

    _cmake = None

    @property
    def _source_subfolder(self):
        return "source_subfolder"

    @property
    def _build_subfolder(self):
        return "build_subfolder"

    def source(self):
        self.run(
            "git clone -b complete git@github.com:jimsch/cn-cbor.git")
        os.rename("cn-cbor", self._source_subfolder)

    def configure(self):
        del self.settings.compiler.libcxx
        del self.settings.compiler.cppstd

    def _configure_cmake(self):
        if not self._cmake:
            self._cmake = CMake(self)
        self._cmake.definitions["build_tests"] = False
        self._cmake.definitions["build_docs"] = False
        self._cmake.definitions["coveralls"] = False
        self._cmake.configure(
            source_folder=self._source_subfolder, build_folder=self._build_subfolder)

        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        self.cpp_info.name = "cn-cbor"
        if self.settings.os == "Linux":
            self.cpp_info.system_libs = ["m"]
