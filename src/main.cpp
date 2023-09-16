// linter doesn't detect compile time definitions
#if !defined(VE001_USE_GLFW3) && !defined(VE001_USE_SDL2)
#define VE001_USE_GLFW3
#endif

#ifdef VE001_USE_GLFW3
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

#include <glad/glad.h>

#include <gl_context.h>
#include <window/window.h>
#include <vmath/vmath.h>

#include <voxel_terrain_generator/voxel_terrain_generator.h>
#include <engine/meshing_engine.h>
#include <engine/chunk_mesh_pool.h>
#include <engine/cubemap.h>
#include <engine/texture_array.h>
#include <engine/material_allocator.h>
#include <sandbox_utils/camera.h>
#include <sandbox_utils/face_gen.h>
#include <engine/shader.h>

#include <logger/logger.h>

#include <cstring>
#include <numbers>

// linter doesn't detect compile time definitions
#ifndef SH_CONFIG_POSITION_ATTRIB_INDEX
#define SH_CONFIG_POSITION_ATTRIB_INDEX 0
#endif
#ifndef SH_CONFIG_TEXCOORD_ATTRIB_INDEX
#define SH_CONFIG_TEXCOORD_ATTRIB_INDEX 1
#endif
#ifndef SH_CONFIG_2D_TEX_ARRAY_BINDING
#define SH_CONFIG_2D_TEX_ARRAY_BINDING 1
#endif
#ifndef SH_CONFIG_MVP_UBO_BINDING
#define SH_CONFIG_MVP_UBO_BINDING 0
#endif
#ifndef SH_CONFIG_MATERIAL_PARAMS_SSBO_BINDING
#define SH_CONFIG_MATERIAL_PARAMS_SSBO_BINDING 2
#endif
#ifndef SH_CONFIG_MATERIAL_DESCRIPTORS_SSBO_BINDING
#define SH_CONFIG_MATERIAL_DESCRIPTORS_SSBO_BINDING 3
#endif

using namespace ve001;
using namespace vmath;

void setVertexLayout(u32 vao, u32 vbo) {
    const u32 vertex_attrib_binding = 0U;

    glEnableVertexArrayAttrib(vao, SH_CONFIG_POSITION_ATTRIB_INDEX);
    glEnableVertexArrayAttrib(vao, SH_CONFIG_TEXCOORD_ATTRIB_INDEX);

    glVertexArrayAttribFormat(vao, SH_CONFIG_POSITION_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribFormat(vao, SH_CONFIG_TEXCOORD_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, texcoord));
    
    glVertexArrayAttribBinding(vao, SH_CONFIG_POSITION_ATTRIB_INDEX, vertex_attrib_binding);
    glVertexArrayAttribBinding(vao, SH_CONFIG_TEXCOORD_ATTRIB_INDEX, vertex_attrib_binding);

    glVertexArrayBindingDivisor(vao, vertex_attrib_binding, 0);

    glVertexArrayVertexBuffer(vao, vertex_attrib_binding, vbo, 0, sizeof(Vertex));
}

void createCubeMapTextureArray(MaterialAllocator& material_allocator, const std::filesystem::path& img_path) {
    CubeMap cubemap{};
    CubeMap::load(cubemap, img_path);

    for (const auto& face : cubemap.faces) {
        material_allocator.addTexture(static_cast<const void*>(face.data()));
    }
}

static constexpr Vec3i32 EXTENT(16, 256, 16);

static f32 timestep{ 0U };
static Camera camera{};

Vec2f32 prev_mouse_pos(0.F);

#ifdef VE001_USE_GLFW3
void keyCallback(GLFWwindow* window_handle, i32 key, i32, i32 action, i32) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        constexpr f32 step{ 24.0 };
        switch(key) {
        case GLFW_KEY_W: camera.move({0.F, 0.F, timestep * step}); break;
        case GLFW_KEY_A: camera.move({timestep * -step, 0.F, 0.F}); break;
        case GLFW_KEY_Q: camera.move({0.F, timestep * -step, 0.F}); break;
        case GLFW_KEY_S: camera.move({0.F, 0.F, timestep * -step}); break;
        case GLFW_KEY_D: camera.move({timestep *  step, 0.F, 0.F}); break;
        case GLFW_KEY_E: camera.move({0.F, timestep *  step, 0.F}); break;
        case GLFW_KEY_L: camera.rotate({0.F,-timestep * std::numbers::pi_v<f32>/2.F, 0.F}); break;
        case GLFW_KEY_H: camera.rotate({0.F, timestep * std::numbers::pi_v<f32>/2.F, 0.F}); break;
        case GLFW_KEY_K: camera.rotate({ timestep * std::numbers::pi_v<f32>/2.F, 0.F, 0.F}); break;
        case GLFW_KEY_J: camera.rotate({-timestep * std::numbers::pi_v<f32>/2.F, 0.F, 0.F}); break;
        case GLFW_KEY_I: camera.rotate({0.F, 0.F, timestep * std::numbers::pi_v<f32>/2.F}); break;
        case GLFW_KEY_O: camera.rotate({0.F, 0.F,-timestep * std::numbers::pi_v<f32>/2.F}); break;
        }
    }
}

void mousePosCallback(GLFWwindow* win_handle, f64 x_pos, f64 y_pos) {
    i32 w{ 0 };
    i32 h{ 0 };
    glfwGetWindowSize(win_handle, &w, &h);
    const auto dw = static_cast<f64>(w);
    const auto dh = static_cast<f64>(h);
    glfwSetCursorPos(win_handle, dw/2.0, dh/2.0);

    const auto aspect_ratio = static_cast<f32>(dw)/static_cast<f32>(dh);

    const auto x_step = static_cast<f32>(x_pos) - (static_cast<f32>(w)/2.F);
    const auto y_step = aspect_ratio * (static_cast<f32>(y_pos) - (static_cast<f32>(h)/2.F));

    static constexpr auto step{ 0.012F };
    camera.rotateXYPlane({timestep * step * x_step, timestep * step * y_step});
}
#endif

int main() {
    if (!ve001::window.init("demo", 640, 480, nullptr)) {
        return 1;
    }

    ve001::glInit();
    // ve001::setGLDebugCallback([](u32, u32, u32 id, u32, int, const char *message, const void *) {
	// 	fmt::print("[GLAD]{}:{}", id, message);
	// });

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

    ve001::ChunkMeshPool chunk_mesh_pool{};
    chunk_mesh_pool.init({
        .chunk_dimensions = EXTENT,
        .max_chunks = 9,
        .vertex_size = sizeof(Vertex),
        .vertex_layout_config = setVertexLayout
    });

    ve001::MeshingEngine meshing_engine({
       .executor = MeshingEngine::Config::Executor::CPU,
       .chunk_size = EXTENT
    });

    ve001::VoxelTerrainGenerator terrain_generator({
        .terrain_size = EXTENT,
        .terrain_density = 4U,
        .noise_type = VoxelTerrainGenerator::Config::NoiseType::PERLIN,
        .quantize_values = 1U,
        .quantized_value_size = sizeof(u8),
        .seed = 0x2223128
    });
    terrain_generator.init();

    std::vector<u8> noise(EXTENT[0] * EXTENT[1] * EXTENT[2], 0U);

    u32 mesh_memory_footprint{ 0U };

    for (i32 z{ 0 }; z < 3; ++z) {
    for (i32 x{ 0 }; x < 3; ++x) {

        terrain_generator.next(static_cast<void*>(noise.data()), 0U, 0U, Vec3i32(x, 0, z));
        
        u32 chunk_id{ 0U };
        chunk_mesh_pool.allocateEmptyChunk(chunk_id, Vec3f32(0.F));

        void* chunk_dst_ptr{ nullptr };
        chunk_mesh_pool.aquireChunkWritePtr(chunk_id, chunk_dst_ptr);

        const auto per_face_vertex_count = meshing_engine.mesh(
            chunk_dst_ptr, 0U, sizeof(Vertex), chunk_mesh_pool.submeshStride(),
            Vec3i32(x, 0, z), 
            [&noise](i32 x, i32 y, i32 z) {
                // return true;
                const auto index = static_cast<std::size_t>(x + y * EXTENT[0] + z * EXTENT[0] * EXTENT[1]);
                return noise[index] > 0U;
            },
            [](void* dst, MeshingEngine::MeshedRegionDescriptor descriptor) -> u32 {
                const auto region_offset = Vec3f32::cast(descriptor.region_offset);
                const auto region_extent = Vec3f32::cast(descriptor.region_extent);
                const auto squashed_region_offset = Vec2f32::cast(descriptor.squashed_region_extent);

                const auto face = genFace(
                    descriptor.face,
                    region_offset,
                    region_extent,
                    squashed_region_offset
                );

                std::memcpy(dst, static_cast<const void*>(face.data()), face.size() * sizeof(Vertex));

                return 6U;
            }
        );

        chunk_mesh_pool.freeChunkWritePtr(chunk_id);

        chunk_mesh_pool.updateChunkSubmeshVertexCounts(chunk_id, per_face_vertex_count);

        mesh_memory_footprint += 
            per_face_vertex_count[0] + 
            per_face_vertex_count[1] +
            per_face_vertex_count[2] +
            per_face_vertex_count[3] +
            per_face_vertex_count[4] +
            per_face_vertex_count[5];
    }}

    mesh_memory_footprint *= sizeof(Vertex);
    //info("mesh memory footprint = {}B = {}kB = {}MB", )
    logger->info("mesh memory footprint = {}B = {}kB = {}MB", 
        mesh_memory_footprint,
        mesh_memory_footprint / 1024,
        mesh_memory_footprint / (1024 * 1024)
    );

    Shader shader{};
    shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/basic_shader/vert.spv", 
        "/home/regu/codium_repos/VE-001/shaders/bin/basic_shader/frag.spv"
    );
    shader.bind();

    MaterialAllocator material_allocator(1U, 1U, 6U, 96U, 96U);

    material_allocator.init();

    material_allocator.addMaterialParams(MaterialParams{
        .color = Vec4f32(1.F),
        .diffuse = Vec3f32(.55F),
        .shininess = .1F,
        .specular = Vec3f32(.2F)
    });

    createCubeMapTextureArray(
        material_allocator,
        "/home/regu/codium_repos/VE-001/assets/textures/mcgrasstexture_small.png"
    );
    
    material_allocator.addMaterial(MaterialDescriptor{
        .material_texture_index = {0U, 1U, 2U, 3U, 4U, 5U},
        .material_params_index = {0U, 0U, 0U, 0U, 0U, 0U}
    });

    material_allocator._texture_rgba8_array.bind(SH_CONFIG_2D_TEX_ARRAY_BINDING);
    material_allocator._material_params_array.bind(SH_CONFIG_MATERIAL_PARAMS_SSBO_BINDING);
    material_allocator._material_descriptors_array.bind(SH_CONFIG_MATERIAL_DESCRIPTORS_SSBO_BINDING);

    u32 ubo{ 0U };
    glCreateBuffers(1, &ubo);
    glNamedBufferStorage(ubo, sizeof(Mat4f32) + sizeof(Vec4f32) + sizeof(Vec4f32), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, SH_CONFIG_MVP_UBO_BINDING, ubo);

#ifdef VE001_USE_GLFW3
    window.setKeyCallback(keyCallback);
    window.setMousePositionCallback(mousePosCallback);
#endif

    glClearColor(.22F, .61F, .78F, 1.F);

    f32 prev_frame_time{ 0.F };
    while(!window.shouldClose()) {
        const auto [window_width, window_height] = window.size();

        prev_mouse_pos[0] = static_cast<f32>(window_width)/2.F;
        prev_mouse_pos[1] = static_cast<f32>(window_height)/2.F;

        const auto frame_time = window.time();
        timestep = frame_time - prev_frame_time;
        prev_frame_time = frame_time;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, window_width, window_height);

        const auto proj_mat = misc<f32>::symmetricPerspectiveProjection(
            .25F * std::numbers::pi_v<f32>, .1F, 100.F, 
            static_cast<f32>(window_width), static_cast<f32>(window_height)
        );

        const auto mvp = Mat4f32::mul(proj_mat, camera.lookAt());

        struct {
            Mat4f32 mvp;
            alignas(sizeof(Vec4f32)) Vec3f32 camera_pos;
            alignas(sizeof(Vec4f32)) Vec3f32 camera_dir;
        } ubo_data {
            .mvp = mvp,
            .camera_pos = camera.position,
            .camera_dir = camera.neg_looking_dir
        };

        glNamedBufferSubData(ubo, 0, sizeof(ubo_data), static_cast<const void*>(&ubo_data));

        chunk_mesh_pool.drawAll();

        window.swapBuffers();
        window.pollEvents();
    }

    glDeleteBuffers(1, &ubo);
    shader.deinit();
    chunk_mesh_pool.deinit();
    material_allocator.deinit();
    window.deinit();

    return 0;
}