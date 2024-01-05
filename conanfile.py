from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

class VE001Recipe(ConanFile):
    build_policy = "missing"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps, CMakeToolchain"

    def config_options(self):
        self.options["glad"].spec = "gl"
        self.options["glad"].extensions = "GL_ARB_gl_spirv"
        self.options["glad"].gl_profile = "core"
        self.options["glad"].gl_version = "4.5"

    def requirements(self):
        self.requires("glad/0.1.36")
        self.requires("spdlog/1.11.0")
        self.requires("cli11/2.3.2")
        if (os != 'Windows'):
            self.requires("fastnoise2/0.10.0-alpha")
        self.requires("glfw/3.3.8")