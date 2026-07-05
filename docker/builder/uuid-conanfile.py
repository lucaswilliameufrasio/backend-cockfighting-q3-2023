from conan import ConanFile

class SysLibUuid(ConanFile):
    name = "util-linux-libuuid"
    version = "2.39.2"
    package_type = "static-library"
    settings = "os", "arch", "compiler", "build_type"

    def requirements(self):
        pass

    def package_info(self):
        self.cpp_info.libs = ["uuid"]
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []
