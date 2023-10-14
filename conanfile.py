from conan import ConanFile

class VE001Recipe(ConanFile):
    build_policy = "missing"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    options = {
        "use_sdl2": [True, False], 
        "use_glfw3": [True, False]
    }

    def config_options(self):
        self.options.use_sdl2 = False
        self.options.use_glfw3 = False

        self.options["sdl"].vulkan = False
        self.options["sdl"].directx = False

        self.options["glad"].spec = "gl"
        self.options["glad"].extensions = "GL_ARB_gl_spirv"
        self.options["glad"].gl_profile = "core"
        self.options["glad"].gl_version = "4.5"

    def requirements(self):
        self.requires("glad/0.1.36")
        self.requires("stb/cci.20220909")
        self.requires("spdlog/1.11.0")
        self.requires("glm/cci.20230113")
        self.requires("cli11/2.3.2")
        self.requires("zstd/1.5.5")
        if self.options.use_sdl2:
            self.requires("sdl/2.26.5")
        if self.options.use_glfw3:    
            self.requires("glfw/3.3.8")