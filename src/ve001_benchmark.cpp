#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <gl_context.h>

#include <window/window.h>
#include <vmath/vmath.h>

#include <engine/engine.h>

#ifndef WINDOWS
#include <sandbox_utils/noise_terrain_generator.h>
#endif

#include <sandbox_utils/simple_terrain_generator.h>
#include <sandbox_utils/camera.h>

#include <logger/logger.h>

#include "testing_context.h"

#include <CLI/CLI.hpp>

/*
    TESTS VARIANTS:
        * G - {simple, noise} x {(400,200,400)} x {2} x {fb} x {64^3} (+2)
        * W - {(200,200,200), (400,200,400), (500,250,500)} x {10} x {noise} x {fb} x {64^3} (+3)
        * O - {none,f,fb}x{(500,200,500)}x{10}x{noise}x{64^3} (+3)
        * S - {10,50,200,1000}x{(400,200,400)}x{noise}x{fb}x{64^3} (+4)
        * C - {32^3,64^3}x{(400,200,400)}x{10}x{noise}x{fb} (+2)
        ___________________________________________________________
            (+14)
*/

//////////////////////////////// CONSTANTS //////////////////////////////////
static constexpr std::string_view vsh_path{ "shaders/bin/basic_test_shader/vert.spv" };
static constexpr std::string_view fsh_path{ "shaders/bin/basic_test_shader/frag.spv" };
//////////////////////////////////////////////////////////////////////////////

////////////////////////////// GLOBAL STATE //////////////////////////////////
static ve001::Camera camera{};
static ve001::Camera sky_camera{};

static bool chosen_camera{ true };
static bool move_camera{ true };

static bool camera_moved{ false };
static bool camera_rotated{ false };
static vmath::f32 timestep{ 0.F };

static bool start_testing{ false };

struct KeyAction {
    bool pressed{false};
    void (*action)(ve001::Camera &chosen_camera, vmath::f32 step);
};

static std::array<KeyAction, 6> keys{{
    {.action = [](ve001::Camera &chosen_camera, vmath::f32 step)
     { chosen_camera.move({0.F, 0.F, timestep * step}); }},
    {.action = [](ve001::Camera &chosen_camera, vmath::f32 step)
     { chosen_camera.move({timestep * -step, 0.F, 0.F}); }},
    {.action = [](ve001::Camera &chosen_camera, vmath::f32 step)
     { chosen_camera.move({0.F, timestep * -step, 0.F}); }},
    {.action = [](ve001::Camera &chosen_camera, vmath::f32 step)
     { chosen_camera.move({0.F, 0.F, timestep * -step}); }},
    {.action = [](ve001::Camera &chosen_camera, vmath::f32 step)
     { chosen_camera.move({timestep * step, 0.F, 0.F}); }},
    {.action = [](ve001::Camera &chosen_camera, vmath::f32 step)
     { chosen_camera.move({0.F, timestep * step, 0.F}); }},
}};

static vmath::Vec2f32 prev_mouse_pos(0.F);
//////////////////////////////////////////////////////////////////////////////

///////////////////////// INPUT ON EVENT FUNCTIONS ///////////////////////////
static void keyCallback(GLFWwindow *win_handle, vmath::i32 key, vmath::i32, vmath::i32 action, vmath::i32);
static void mousePosCallback(GLFWwindow *win_handle, vmath::f64 x_pos, vmath::f64 y_pos);
//////////////////////////////////////////////////////////////////////////////

static vmath::u32 getBackFaceCullMask(const ve001::Camera& camera) {
    static constexpr vmath::f32 epsilon{ .05F };

    vmath::u32 result{ 0U };
    for (vmath::u32 i{ 0UL }; i < 3UL; ++i) {
        if (camera.looking_dir[i] > -epsilon) {
            result |= (1U << (i * 2U + 1U));
        }
        if (camera.looking_dir[i] < epsilon) {
            result |= (1U << (i * 2U + 0U));
        }
    }

    return result;
}

static constexpr std::array<vmath::Vec3f32, 6> FACE_NORMAL_LOOKUP_TABLE {{
    { 1.F, 0.F, 0.F}, 
    {-1.F, 0.F, 0.F},
    { 0.F, 1.F, 0.F},
    { 0.F,-1.F, 0.F},
    { 0.F, 0.F, 1.F},
    { 0.F, 0.F,-1.F}
}};

static bool backFaceCullingUnaryOp(
    ve001::Face orientation, 
    vmath::Vec3f32 position,
    vmath::Vec3f32 half_chunk_size,
    vmath::u32 camera_mask,
    vmath::Vec3f32 camera_position) {
    
    if ((camera_mask & (1U << orientation)) > 0U) {
        return true;
    }

    static constexpr vmath::f32 POINTS_DENSITY{ .5F };
    static constexpr vmath::f32 BIAS{ .1F };

    for (vmath::f32 z{ -.5F }; z < .5F; z += POINTS_DENSITY ) {
    for (vmath::f32 y{ -.5F }; y < .5F; y += POINTS_DENSITY ) {
    for (vmath::f32 x{ -.5F }; x < .5F; x += POINTS_DENSITY ) {

        const auto point = vmath::Vec3f32::mul({x, y, z}, half_chunk_size);
        const auto normal = vmath::Vec3f32::normalize(vmath::Vec3f32::sub(vmath::Vec3f32::add(position, point), camera.position));

        if (vmath::Vec3f32::dot(normal, FACE_NORMAL_LOOKUP_TABLE[orientation]) <= BIAS) {
            return true;
        }
    }
    }
    }

    return false;
}

struct CLIAppConfig {
#ifndef WINDOWS
    bool simple_generator{ false };
#endif
    vmath::Vec3i32 chunk_size{ 0 };
    bool frustum_culling{ false };
    bool back_face_culling{ false };
    vmath::i32 number_of_streamer_threads{ 0 };
    vmath::Vec3f32 world_size{ 100.F, 100.F, 100.F };
    vmath::i32 voxel_states_count; 
};

static TestingContext testing_context(5000);

int main(int argc, const char* const* argv) {
    CLI::App app("CLI app for running benchmarks on ve001 engine", "ve001-benchmark");

    CLIAppConfig cli_app_config{};
#ifndef WINDOWS
    app.add_flag("-s,--simple-generator", cli_app_config.simple_generator, "switch from noise to simple data generator");
#endif
    app.add_flag("-f,--frustum-culling", cli_app_config.frustum_culling, "turn on frustum culling");
    app.add_flag("-b,--backface-culling", cli_app_config.back_face_culling, "turn on backface culling");
    app.add_option("-t,--threads-count", cli_app_config.number_of_streamer_threads, "number of threads used by chunk data streamer");
    app.add_option("-w,--voxel-states-count", cli_app_config.voxel_states_count, "number of voxel states used")->required();
    app.add_option("-x,--chunk-size-x", cli_app_config.chunk_size[0], "size of chunk in X axis")->required();
    app.add_option("-y,--chunk-size-y", cli_app_config.chunk_size[1], "size of chunk in Y axis")->required();
    app.add_option("-z,--chunk-size-z", cli_app_config.chunk_size[2], "size of chunk in Z axis")->required();
    app.add_option("-X,--world-size-x", cli_app_config.world_size[0], "size of world in X axis")->required();
    app.add_option("-Y,--world-size-y", cli_app_config.world_size[1], "size of world in Y axis")->required();
    app.add_option("-Z,--world-size-z", cli_app_config.world_size[2], "size of world in Z axis")->required();

    CLI11_PARSE(app, argc, argv);
    
    if (!ve001::window.init("ve001-benchmark", 640, 480, nullptr)) {
        return 1;
    }
    ve001::glInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(.22F, .61F, .78F, 1.F);

    ve001::window.setKeyCallback(keyCallback);
    ve001::window.setMousePositionCallback(mousePosCallback);

    testing_context.init();

    ve001::Engine engine({
        .world_size = cli_app_config.world_size,
        .initial_position = {0.F, 0.F, 0.F},
        .chunk_size = cli_app_config.chunk_size,
#ifndef WINDOWS
        .chunk_data_generator = cli_app_config.simple_generator ?
            std::unique_ptr<ve001::ChunkGenerator>(
                new ve001::SimpleTerrainGenerator(cli_app_config.chunk_size)
            ) :
            std::unique_ptr<ve001::ChunkGenerator>(
                new ve001::NoiseTerrainGenerator({
                    .terrain_size = cli_app_config.chunk_size,
                    .noise_frequency = .0038F,
                    .quantize_values = cli_app_config.voxel_states_count,
                    .seed = 0xC0000B99,
                    .visibilty_threshold = .3F,
                })
            )
#else
        .chunk_data_generator = std::unique_ptr<ve001::ChunkGenerator>(
            new ve001::SimpleTerrainGenerator(cli_app_config.chunk_size)
        )
#endif
        ,
        .chunk_pool_growth_coefficient = 1.5F,
        .meshing_shader_local_group_size = 64
    });
    engine.init();

    ve001::Shader shader;
    shader.init();
    shader.attach(vsh_path, fsh_path, true);

    struct General {
        vmath::Mat4f32 vp;
        alignas(16) vmath::Vec3f32 camera_pos;
    } general_data;
    ve001::GPUBuffer general_ubo(sizeof(General));
    general_ubo.init();
    general_ubo.bind(GL_UNIFORM_BUFFER, 0);

    engine.partitioning = (cli_app_config.back_face_culling || cli_app_config.frustum_culling);

    const auto half_chunk_size = vmath::Vec3f32::divScalar(vmath::Vec3f32::cast(cli_app_config.chunk_size), 2.F);

    vmath::f32 prev_frame_time{0.F};
    while (!ve001::window.shouldClose()) {
        const auto [window_width, window_height] = ve001::window.size();

        const auto frame_time = ve001::window.time();
        timestep = frame_time - prev_frame_time;
        prev_frame_time = frame_time;

        for (auto &key : keys) {
            if (key.pressed) {
                key.action(move_camera ? camera : sky_camera, 30.F);
                if (move_camera) {
                    camera_moved = true;
                }
            }
        }

        static constexpr vmath::f32 CAMERA_Z_NEAR{ .1F };
        static constexpr vmath::f32 CAMERA_Z_FAR{ 500.F };
        static constexpr vmath::f32 CAMERA_FOV{ .5F * std::numbers::pi_v<vmath::f32> };
        static constexpr vmath::f32 CAMERA_FOV_BIASED{ .65F * std::numbers::pi_v<vmath::f32> };

        const auto proj_mat = chosen_camera ? 
            vmath::misc<vmath::f32>::symmetricPerspectiveProjection(
                CAMERA_FOV, CAMERA_Z_NEAR, CAMERA_Z_FAR,
                static_cast<vmath::f32>(window_width), static_cast<vmath::f32>(window_height)
            ) :
            vmath::misc<vmath::f32>::symmetricOrthographicProjection(
                .1F, 1000.F, static_cast<vmath::f32>(window_width), static_cast<vmath::f32>(window_height)
            );


        if (start_testing) {
            testing_context.beginMeasure();
        }

        general_data.vp = vmath::Mat4f32::mul(proj_mat, (chosen_camera ? camera : sky_camera).lookAt());
        general_data.camera_pos = camera.position;
        general_ubo.write(static_cast<const void*>(&general_data));

        engine.updateCameraPosition(camera.position);
        
        if (camera_moved || camera_rotated) {

            if (cli_app_config.frustum_culling) {
                static const auto TAN_FOV = std::tan(CAMERA_FOV_BIASED/2.F);
                const auto aspect_ratio = (static_cast<vmath::f32>(window_width)/static_cast<vmath::f32>(window_height));

                engine.applyFrustumCullingPartition(
                    false,
                    -CAMERA_Z_NEAR,
                    -CAMERA_Z_FAR,
                    aspect_ratio * CAMERA_Z_NEAR * TAN_FOV,
                    CAMERA_Z_NEAR * TAN_FOV,
                    camera.lookAt()
                );
            }

            if (cli_app_config.back_face_culling) {

                engine.applyCustomPartition(
                    backFaceCullingUnaryOp,
                    cli_app_config.frustum_culling,
                    half_chunk_size,
                    getBackFaceCullMask(camera),
                    camera.position
                );
            }

            camera_moved = false;
            camera_rotated = false;
        }

        if (engine.pollChunksUpdates() && start_testing) {
            testing_context.saveMeshingSample({
                engine._world_grid._chunk_pool._meshing_engine.result_gpu_meshing_time_ns,
                engine._world_grid._chunk_pool._meshing_engine.result_real_meshing_time_ns
            });
        }

        engine.updateDrawState();

        // main render
        glViewport(0, 0, window_width, window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.bind();

        engine.draw();

        if (start_testing) {
            testing_context.endMeasure(
                engine._world_grid._chunk_pool.chunks_used,
                engine._world_grid._chunk_pool.gpu_memory_usage,
                engine._world_grid._chunk_pool.chunks_used * engine._engine_context.chunk_max_current_mesh_size,
                static_cast<vmath::u64>(engine._world_grid._chunk_pool._chunks_count) * engine._engine_context.chunk_max_current_mesh_size,
                engine._world_grid._chunk_pool.cpu_active_memory_usage,
                static_cast<vmath::u64>(engine._world_grid._chunk_pool._chunks_count) * engine._engine_context.chunk_voxel_data_size
            );
        }

        ve001::window.swapBuffers();
        ve001::window.pollEvents();
    }

    if (start_testing) {
        testing_context.dumpFrameSamples();
        testing_context.dumpMeshingSamples();
    }

    testing_context.deinit();

    general_ubo.deinit();
    shader.deinit();
    engine.deinit();
}

//////////////////////// INPUT ON EVENT FUNCTIONS cd. ////////////////////////
void keyCallback(GLFWwindow *win_handle, vmath::i32 key, vmath::i32, vmath::i32 action, vmath::i32) {
    switch (key)
    {
    case GLFW_KEY_W:
        keys[0].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        break;
    case GLFW_KEY_A:
        keys[1].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        break;
    case GLFW_KEY_Q:
        keys[2].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        break;
    case GLFW_KEY_S:
        keys[3].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        break;
    case GLFW_KEY_D:
        keys[4].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        break;
    case GLFW_KEY_E:
        keys[5].pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
        break;
    case GLFW_KEY_U:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT)) {
            chosen_camera = !chosen_camera;
        }
        break;
    case GLFW_KEY_M:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT)) {
            move_camera = !move_camera;
        }
        break;
    case GLFW_KEY_P:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT)) {
            start_testing = true;
        }
        break;
    }    
}
void mousePosCallback(GLFWwindow *win_handle, vmath::f64 x_pos, vmath::f64 y_pos) {
    vmath::i32 w{0};
    vmath::i32 h{0};
    glfwGetWindowSize(win_handle, &w, &h);
    const auto dw = static_cast<vmath::f64>(w);
    const auto dh = static_cast<vmath::f64>(h);
    if (x_pos == dw / 2.0 && y_pos == dh / 2.0) {
        return;
    }
    glfwSetCursorPos(win_handle, dw / 2.0, dh / 2.0);

    const auto aspect_ratio = static_cast<vmath::f32>(dw) / static_cast<vmath::f32>(dh);

    const auto x_step = (static_cast<vmath::f32>(x_pos) + prev_mouse_pos[0]) / 2.F - (static_cast<vmath::f32>(w) / 2.F);
    const auto y_step = aspect_ratio * ((static_cast<vmath::f32>(y_pos) + prev_mouse_pos[1]) / 2.F - (static_cast<vmath::f32>(h) / 2.F));

    static constexpr auto step{0.020F};

    auto& c = move_camera ? camera : sky_camera;
    c.rotateXYPlane({timestep * step * x_step, timestep * step * y_step});

    if (move_camera) {
        camera_rotated = true;
    }
    prev_mouse_pos[0] = static_cast<vmath::f32>(x_pos);
    prev_mouse_pos[1] = static_cast<vmath::f32>(y_pos);
}
//////////////////////////////////////////////////////////////////////////////
