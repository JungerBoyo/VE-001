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
#include <engine/camera.h>
#include <sandbox_utils/face_gen.h>
#include <engine/shader.h>
#include <engine/framebuffer.h>
#include <engine/lighting.h>
#include <engine/gpu_buffer.h>

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
#ifndef SH_CONFIG_2D_SHADOW_MAP_TEX_BINDING
#define SH_CONFIG_2D_SHADOW_MAP_TEX_BINDING 5
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

static constexpr Vec3i32 EXTENT(16, 32, 16);

static f32 timestep{ 0U };
static Camera camera{};
// static Vec3f32 light_pos(1.F, 0.F, 0.F); // ~light_pos~ light_dir
static Camera light{};
bool light_as_camera{ false };
bool light_as_camera_proj{ false };
bool camera_rotated{ false };
bool camera_moved{ false };

Vec2f32 prev_mouse_pos(0.F);

struct KeyAction {
    bool pressed{ false };
    void(*action)(Camera& chosen_camera, f32 step);
};

std::array<KeyAction, 6> keys {{
    { .action = [](Camera& chosen_camera, f32 step) { chosen_camera.move({0.F, 0.F, timestep *  step}); } },  
    { .action = [](Camera& chosen_camera, f32 step) { chosen_camera.move({timestep * -step, 0.F, 0.F}); } },
    { .action = [](Camera& chosen_camera, f32 step) { chosen_camera.move({0.F, timestep * -step, 0.F}); } },
    { .action = [](Camera& chosen_camera, f32 step) { chosen_camera.move({0.F, 0.F, timestep * -step}); } },
    { .action = [](Camera& chosen_camera, f32 step) { chosen_camera.move({timestep *  step, 0.F, 0.F}); } },
    { .action = [](Camera& chosen_camera, f32 step) { chosen_camera.move({0.F, timestep *  step, 0.F}); } },
}};

#ifdef VE001_USE_GLFW3
void keyCallback(GLFWwindow* window_handle, i32 key, i32, i32 action, i32) {
    switch(key) {
    case GLFW_KEY_W: keys[0].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT); break;
    case GLFW_KEY_A: keys[1].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT); break;
    case GLFW_KEY_Q: keys[2].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT); break;
    case GLFW_KEY_S: keys[3].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT); break;
    case GLFW_KEY_D: keys[4].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT); break;
    case GLFW_KEY_E: keys[5].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT); break;

    case GLFW_KEY_U: if (action == GLFW_PRESS || action == GLFW_REPEAT) { light_as_camera = !light_as_camera; } break;
    case GLFW_KEY_I: if (action == GLFW_PRESS || action == GLFW_REPEAT) { light_as_camera_proj = !light_as_camera_proj; } break;
    }
}

void mousePosCallback(GLFWwindow* win_handle, f64 x_pos, f64 y_pos) {
    i32 w{ 0 };
    i32 h{ 0 };
    glfwGetWindowSize(win_handle, &w, &h);
    const auto dw = static_cast<f64>(w);
    const auto dh = static_cast<f64>(h);
    if (x_pos == dw/2.0 && y_pos == dh/2.0) {
        return;
    }
    glfwSetCursorPos(win_handle, dw/2.0, dh/2.0);

    const auto aspect_ratio = static_cast<f32>(dw)/static_cast<f32>(dh);

    const auto x_step = (static_cast<f32>(x_pos) + prev_mouse_pos[0])/2.F - (static_cast<f32>(w)/2.F);
    const auto y_step = aspect_ratio * ((static_cast<f32>(y_pos) + prev_mouse_pos[1])/2.F - (static_cast<f32>(h)/2.F));

    static constexpr auto step{ 0.020F };

    auto& chosen_camera = light_as_camera ? light : camera;

    chosen_camera.rotateXYPlane({timestep * step * x_step, timestep * step * y_step});
    camera_rotated = true;

    prev_mouse_pos[0] = static_cast<f32>(x_pos);
    prev_mouse_pos[1] = static_cast<f32>(y_pos);

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
        .max_chunks = 26,
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

    std::array<Vec3i32, 26> chunk_positions{{
        {0, 0, 0}, {1, 0, 0}, {2, 0, 0},
        {0, 0, 1}, {1, 0, 1}, {2, 0, 1},
        {0, 0, 2}, {1, 0, 2}, {2, 0, 2},
        {1, 1, 1},
        {0, 0, -1}, {1, 0, -1}, {2, 0, -1},
        {0, 0, 3}, {1, 0, 3}, {2, 0, 3},
        {-1, 0, 0}, {-1, 0, 1}, {-1, 0, 2},
        {3, 0, 0}, {3, 0, 1}, {3, 0, 2},
    }};

    u32 mesh_memory_footprint{ 0U };
    for (const auto chunk_position : chunk_positions) {
        terrain_generator.next(static_cast<void*>(noise.data()), 0U, 0U, chunk_position);
        
        u32 chunk_id{ 0U };
        chunk_mesh_pool.allocateEmptyChunk(chunk_id, Vec3f32(0.F));

        void* chunk_dst_ptr{ nullptr };
        chunk_mesh_pool.aquireChunkWritePtr(chunk_id, chunk_dst_ptr);

        const auto per_face_vertex_count = meshing_engine.mesh(
            chunk_dst_ptr, 0U, sizeof(Vertex), chunk_mesh_pool.submeshStride(),
            chunk_position, 
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
    }

    mesh_memory_footprint *= sizeof(Vertex);
    //info("mesh memory footprint = {}B = {}kB = {}MB", )
    logger->info("mesh memory footprint = {}B = {}kB = {}MB", 
        mesh_memory_footprint,
        mesh_memory_footprint / 1024,
        mesh_memory_footprint / (1024 * 1024)
    );

    // Shader shader{};
    // shader.attach(
    //     "/home/regu/codium_repos/VE-001/shaders/bin/basic_shader/vert.spv", 
    //     "/home/regu/codium_repos/VE-001/shaders/bin/basic_shader/frag.spv"
    // );

    // Shader shadow_map_shader{};
    // shadow_map_shader.attach(
    //     "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_shader/vert.spv", 
    //     "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_shader/frag.spv"
    // );

    Shader shader{};
    shader.init();
    shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/multi_lights_shader/vert.spv", 
        "/home/regu/codium_repos/VE-001/shaders/bin/multi_lights_shader/frag.spv"
    );

    MaterialAllocator material_allocator(1U, 1U, 6U, 96U, 96U);

    material_allocator.init();

    material_allocator.addMaterialParams(MaterialParams{
        .specular = Vec4f32(1.F, 1.F, 1.F, 32.F),
        .diffuse = Vec3f32(1.F),
        .alpha = 1.F
    });

    createCubeMapTextureArray(
        material_allocator,
        "/home/regu/codium_repos/VE-001/assets/textures/mcgrasstexture_small.png"
    );
    
    material_allocator.addMaterial(MaterialDescriptor{
        .material_texture_index = {0U, 1U, 2U, 3U, 4U, 5U},
        .material_params_index  = {0U, 0U, 0U, 0U, 0U, 0U}
    });

    material_allocator._texture_rgba8_array.bind(VE001_SH_CONFIG_TEX_BINDING_MATERIAL_TEXTURES);
    material_allocator._material_params_array.bind(VE001_SH_CONFIG_SSBO_BINDING_MATERIAL_PARAMS);
    material_allocator._material_descriptors_array.bind(VE001_SH_CONFIG_SSBO_BINDING_MATERIAL_DESCRIPTORS);

#ifdef VE001_USE_GLFW3
    window.setKeyCallback(keyCallback);
    window.setMousePositionCallback(mousePosCallback);
#endif

    glClearColor(.22F, .61F, .78F, 1.F);
    
    struct General {
        Mat4f32 mvp;
        alignas(16) Vec3f32 camera_pos;
        alignas(16) Vec3f32 camera_dir;
    } general;

    GPUBuffer general_ubo(sizeof(General));
    general_ubo.init();
    general_ubo.bind(GL_UNIFORM_BUFFER, VE001_SH_CONFIG_UBO_BINDING_GENERAL);

    glEnable(GL_FRAMEBUFFER_SRGB);

    Lighting lighting(1024, 1024);
    lighting.init();

    lighting._directional._buffer.bind(VE001_SH_CONFIG_SSBO_BINDING_DIRECTIONAL_LIGHTS);
    lighting._point._buffer.bind(VE001_SH_CONFIG_SSBO_BINDING_POINT_LIGHTS);
    lighting._spot._buffer.bind(VE001_SH_CONFIG_SSBO_BINDING_SPOT_LIGHTS);
    lighting._descriptor_buffer.bind(GL_UNIFORM_BUFFER, VE001_SH_CONFIG_UBO_BINDING_LIGHTS_DESCRIPTOR);
    lighting._shadow_map_array.bind(VE001_SH_CONFIG_TEX_BINDING_DIR_SHADOW_MAPS);
    lighting._cube_shadow_map_array.bind(VE001_SH_CONFIG_TEX_BINDING_OMNI_DIR_SHADOW_MAPS);


    const auto dir_light_id = lighting.addPointLight({
        .params = {
            .position = Vec3f32(0.F, 0.F, 0.F),
            .ambient = Vec3f32(.1F),
            .diffuse = Vec3f32(.55F, .48F, .42F),
            .specular = Vec3f32(1.F, 1.F, 1.F),
            .attenuation_params = Vec3f32(2.F, .009F, .0032F)
        },
        .shadow_casting = true
    });
    // const auto dir_light_id = lighting.addSpotLight({
    //     .params = {
    //         .position = Vec3f32(0.F, 0.F, 0.F),
    //         .direction = Vec3f32(0.F, 0.F, 1.F),
    //         .ambient = Vec3f32(.3F, .1F, .1F),
    //         .diffuse = Vec3f32(.7F, .42F, .42F),
    //         .specular = Vec3f32(1.F, 1.F, 1.F),
    //         .attenuation_params = Vec3f32(2.F, .009F, .0032F),
    //         .cut_off = Vec3f32(
    //             std::cos(std::numbers::pi_v<f32>/8.F), 
    //             std::cos(std::numbers::pi_v<f32>/6.4F), 
    //             std::cos(std::numbers::pi_v<f32>/8.F) - std::cos(std::numbers::pi_v<f32>/6.4F)
    //         )
    //     },
    //     .shadow_casting = true 
    // });

    // light.move({-20.F, 40.F, 20.F});
    camera.move({0.F, 40.F, 6.F});  
    

    const auto render_scene = [&chunk_mesh_pool]() {
        chunk_mesh_pool.drawAll();
    };


    f32 prev_frame_time{ 0.F };
    while(!window.shouldClose()) {
        const auto [window_width, window_height] = window.size();

        const auto frame_time = window.time();
        timestep = frame_time - prev_frame_time;
        prev_frame_time = frame_time;

        for (auto& key : keys) {
            if (key.pressed) {
                key.action(light_as_camera ? light : camera, 10.F);
                camera_moved = true;
            }
        }

        const auto proj_mat = misc<f32>::symmetricPerspectiveProjection(
            .4F * std::numbers::pi_v<f32>, .1F, 1000.F, 
            static_cast<f32>(window_width), static_cast<f32>(window_height)
        );

        const auto mvp = Mat4f32::mul(proj_mat, camera.lookAt());

        // const auto light_proj_mat = misc<f32>::symmetricOrthographicProjection(
        //     .1F, 1000.F, 
        //     static_cast<f32>(window_width), static_cast<f32>(window_height)
        // );
        const auto light_proj_mat = misc<f32>::symmetricPerspectiveProjection(
            .5F * std::numbers::pi_v<f32> /*1.41F * std::numbers::pi_v<f32>/7.4F*/, .1F, 100.F, 
            static_cast<f32>(window_width), static_cast<f32>(window_height)
        );
        general.mvp =  light_as_camera_proj ? Mat4f32::mul(light_proj_mat, light.lookAt()) : mvp;
        general.camera_pos = camera.position;
        general.camera_dir = camera.looking_dir;
            // .light_dir = -light.looking_dir 

        general_ubo.write(static_cast<const void*>(&general));

        chunk_mesh_pool.update();

        if (camera_rotated && light_as_camera) {
            const auto dir_light_rot_angles = lighting.getLightRotation(dir_light_id);
            lighting.rotateLight(dir_light_id, light.looking_dir);//Vec3f32::sub(light.looking_dir_angles, dir_light_rot_angles));
            camera_rotated = false;
        }
        if (camera_moved && light_as_camera) {
            lighting.moveLight(dir_light_id, light.position);
            camera_moved = false;
        }
        // render shadow map
        if (lighting.update(render_scene)) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glTextureBarrier(); // barrier since depth texture is used in subsequent draw
        }
        // main render
        glViewport(0, 0, window_width, window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.bind();
        chunk_mesh_pool.drawAll();

        window.swapBuffers();
        window.pollEvents();
    }

    general_ubo.deinit();
    shader.deinit();
    lighting.deinit();
    chunk_mesh_pool.deinit();
    material_allocator.deinit();
    window.deinit();


    return 0;
}