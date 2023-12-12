#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <gl_context.h>

#include <window/window.h>
#include <vmath/vmath.h>

#include <engine/engine.h>

#include <sandbox_utils/noise_terrain_generator.h>
#include <sandbox_utils/simple_terrain_generator.h>
#include <sandbox_utils/camera.h>

#include <logger/logger.h>

#include "testing_context.h"

#include <CLI/CLI.hpp>

/*
    G - simple or noise
    C - 32x32x32 or 64x64x64
    P - with or without (frustum or (frustum and backface culling))
*/

//////////////////////////////// CONSTANTS //////////////////////////////////
// static constexpr vmath::Vec3i32 CHUNK_SIZE(64, 64, 64);
// static constexpr vmath::Vec3f32 WORLD_SIZE(400.F, 100.F, 400.F);

static constexpr std::string_view vsh_path{ "shaders/bin/basic_test_shader/vert.spv" };
static constexpr std::string_view fsh_path{ "shaders/bin/basic_test_shader/frag.spv" };
//////////////////////////////////////////////////////////////////////////////

////////////////////////////// GLOBAL STATE //////////////////////////////////
static ve001::Camera camera{};
static ve001::Camera sky_camera{};

static bool chosen_camera{ false };

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

struct CLIAppConfig {
    bool simple_generator{ false };
    vmath::Vec3i32 chunk_size{ 0 };
    bool frustum_culling{ false };
    bool back_face_culling{ false };
    vmath::i32 number_of_streamer_threads{ 0 };
    vmath::Vec3f32 world_size{ 100.F, 100.F, 100.F };        
};

int main(int argc, const char* const* argv) {
    CLI::App app("CLI app for running benchmarks on ve001 engine", "ve001-benchmark");

    CLIAppConfig cli_app_config{};
    app.add_flag("-s,--simple-generator", cli_app_config.simple_generator, "switch from noise to simple data generator");
    app.add_flag("-f,--frustum-culling", cli_app_config.frustum_culling, "turn on frustum culling");
    app.add_flag("-b,--backface-culling", cli_app_config.back_face_culling, "turn on backface culling");
    app.add_option("-t,--threads-count", cli_app_config.number_of_streamer_threads, "number of threads used by chunk data streamer");
    app.add_option("-x,--chunk-size-x", cli_app_config.chunk_size[0], "size of chunk in X axis")->required();
    app.add_option("-y,--chunk-size-y", cli_app_config.chunk_size[1], "size of chunk in Y axis")->required();
    app.add_option("-z,--chunk-size-z", cli_app_config.chunk_size[2], "size of chunk in Z axis")->required();
    app.add_option("-X,--world-size-x", cli_app_config.world_size[0], "size of world in X axis")->required();
    app.add_option("-Y,--world-size-y", cli_app_config.world_size[1], "size of world in Y axis")->required();
    app.add_option("-Z,--world-size-z", cli_app_config.world_size[2], "size of world in Z axis")->required();

    CLI11_PARSE(app, argc, argv);
    
    if (!ve001::window.init("basic test", 640, 480, nullptr)) {
        return 1;
    }
    ve001::glInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(.22F, .61F, .78F, 1.F);

    ve001::window.setKeyCallback(keyCallback);
    ve001::window.setMousePositionCallback(mousePosCallback);

    ve001::Engine engine({
        .world_size = cli_app_config.world_size,
        .initial_position = {0.F, 0.F, 0.F},
        .chunk_size = cli_app_config.chunk_size,
        .chunk_data_generator = cli_app_config.simple_generator ?
            std::unique_ptr<ve001::ChunkGenerator>(
                new ve001::SimpleTerrainGenerator(cli_app_config.chunk_size)
            ) :
            std::unique_ptr<ve001::ChunkGenerator>(
                new ve001::NoiseTerrainGenerator({
                    .terrain_size = cli_app_config.chunk_size,
                    .noise_frequency = .0048F,
                    .quantize_values = 257U,
                    .seed = 0xC0000B99,
                    .visibilty_threshold = .3F
                })
            )
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

    engine.partitioning = false;

    TestingContext testing_context;
    testing_context.init();

    vmath::f32 prev_frame_time{0.F};
    while (!ve001::window.shouldClose()) {
        const auto [window_width, window_height] = ve001::window.size();

        const auto frame_time = ve001::window.time();
        timestep = frame_time - prev_frame_time;
        prev_frame_time = frame_time;

        for (auto &key : keys) {
            if (key.pressed) {
                key.action(chosen_camera ? camera : sky_camera, 20.F);
                camera_moved = true;
            }
        }

        const auto proj_mat = chosen_camera ? 
            vmath::misc<vmath::f32>::symmetricPerspectiveProjection(
                .5F * std::numbers::pi_v<vmath::f32>, .1F, 500.F,
                static_cast<vmath::f32>(window_width), static_cast<vmath::f32>(window_height)
            ) :
            vmath::misc<vmath::f32>::symmetricOrthographicProjection(
                .1F, 1000.F,
                static_cast<vmath::f32>(window_width), static_cast<vmath::f32>(window_height)
            );


        if (start_testing) {
            testing_context.beginMeasure();
        }
        
        general_data.vp = vmath::Mat4f32::mul(proj_mat, (chosen_camera ? camera : sky_camera).lookAt());
        general_data.camera_pos = camera.position;
        general_ubo.write(static_cast<const void*>(&general_data));

        engine.updateCameraPosition(camera.position);
        
        engine.pollChunksUpdates();

        if (camera_moved || camera_rotated) {
            camera_moved = false;
            camera_rotated = false;
        }

        engine.updateDrawState();

        // main render
        glViewport(0, 0, window_width, window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.bind();

        engine.draw();

        if (start_testing) {
            testing_context.endMeasure();
        }

        ve001::window.swapBuffers();
        ve001::window.pollEvents();
    }

    ve001::logger->info(R"(
        For number of frames rendered {}:
            Mean CPU time elapsed was {}ms
            Max CPU time elapsed was {}ms
            Mean GPU time elapsed was {}ms
            Max GPU time elapsed was {}ms
            Mean samples passed was {}
            Mean primitives generated was {}
            Mean number of quads generated was {}
            Mean chunk pool passive GPU memory usage was {}MB
            Mean chunk pool active GPU memory usage was {}MB
            Mean chunk pool CPU memory usage was {}MB
            Mean number of chunks allocated was {}

        )",
        testing_context.frame_counter,
        static_cast<vmath::f64>(testing_context.cpu_mean_frame_time_elapsed_ns) / 1'000'000.F,
        static_cast<vmath::f64>(testing_context.cpu_max_frame_time_elapsed_ns) / 1'000'000.F,
        static_cast<vmath::f64>(testing_context.gpu_mean_frame_time_elapsed_ns) / 1'000'000.F,
        static_cast<vmath::f64>(testing_context.gpu_max_frame_time_elapsed_ns) / 1'000'000.F,
        testing_context.mean_samples_passed,
        testing_context.mean_prims_generated,
        testing_context.mean_prims_generated/4,
        static_cast<vmath::f32>(engine._world_grid._chunk_pool.chunks_used * engine._engine_context.chunk_max_current_mesh_size) / (1024.F*1024.F),
        static_cast<vmath::f32>(engine._world_grid._chunk_pool.gpu_memory_usage) / (1024.F * 1024.F),
        static_cast<vmath::f32>(engine._world_grid._chunk_pool.cpu_memory_usage) / (1024.F * 1024.F),
        engine._world_grid._chunk_pool.chunks_used
    );

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

    auto& c = chosen_camera ? camera : sky_camera;
    c.rotateXYPlane({timestep * step * x_step, timestep * step * y_step});
    camera_rotated = true;

    prev_mouse_pos[0] = static_cast<vmath::f32>(x_pos);
    prev_mouse_pos[1] = static_cast<vmath::f32>(y_pos);
}
//////////////////////////////////////////////////////////////////////////////