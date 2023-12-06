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

#include <engine/engine.h>
#include <engine/cubemap.h>
#include <engine/texture_array.h>
#include <engine/material_allocator.h>
#include <engine/camera.h>
// #include <sandbox_utils/face_gen.h>
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

void createCubeMapTextureArray(MaterialAllocator &material_allocator, const std::filesystem::path &img_path)
{
    CubeMap cubemap{};
    CubeMap::load(cubemap, img_path);

    for (const auto &face : cubemap.faces)
    {
        material_allocator.addTexture(static_cast<const void *>(face.data()));
    }
}

static constexpr Vec3i32 EXTENT(64, 64, 64);
static constexpr Vec3f32 EXTENT_F32(64.F, 64.F, 64.F);

static constexpr std::array<Vec3f32, 8> CORNERS{{
    {-static_cast<f32>(EXTENT[0]/2), -static_cast<f32>(EXTENT[1]/2), -static_cast<f32>(EXTENT[2]/2)},
    {-static_cast<f32>(EXTENT[0]/2), -static_cast<f32>(EXTENT[1]/2),  static_cast<f32>(EXTENT[2]/2)},
    {-static_cast<f32>(EXTENT[0]/2),  static_cast<f32>(EXTENT[1]/2), -static_cast<f32>(EXTENT[2]/2)},
    {-static_cast<f32>(EXTENT[0]/2),  static_cast<f32>(EXTENT[1]/2),  static_cast<f32>(EXTENT[2]/2)},
    { static_cast<f32>(EXTENT[0]/2), -static_cast<f32>(EXTENT[1]/2), -static_cast<f32>(EXTENT[2]/2)},
    { static_cast<f32>(EXTENT[0]/2), -static_cast<f32>(EXTENT[1]/2),  static_cast<f32>(EXTENT[2]/2)},
    { static_cast<f32>(EXTENT[0]/2),  static_cast<f32>(EXTENT[1]/2), -static_cast<f32>(EXTENT[2]/2)},
    { static_cast<f32>(EXTENT[0]/2),  static_cast<f32>(EXTENT[1]/2),  static_cast<f32>(EXTENT[2]/2)},
}};

// struct Plane {
//     Vec3f32 normal
// };

static f32 timestep{0U};
static Camera camera{};
// static Vec3f32 light_pos(1.F, 0.F, 0.F); // ~light_pos~ light_dir
static Camera light{};
bool light_as_camera{false};
bool light_as_camera_proj{false};
bool camera_rotated{false};
bool camera_moved{false};

i32 light_selector{0};
bool light_changed{false};

Vec2f32 prev_mouse_pos(0.F);

struct KeyAction
{
    bool pressed{false};
    void (*action)(Camera &chosen_camera, f32 step);
};

std::array<KeyAction, 6> keys{{
    {.action = [](Camera &chosen_camera, f32 step)
     { chosen_camera.move({0.F, 0.F, timestep * step}); }},
    {.action = [](Camera &chosen_camera, f32 step)
     { chosen_camera.move({timestep * -step, 0.F, 0.F}); }},
    {.action = [](Camera &chosen_camera, f32 step)
     { chosen_camera.move({0.F, timestep * -step, 0.F}); }},
    {.action = [](Camera &chosen_camera, f32 step)
     { chosen_camera.move({0.F, 0.F, timestep * -step}); }},
    {.action = [](Camera &chosen_camera, f32 step)
     { chosen_camera.move({timestep * step, 0.F, 0.F}); }},
    {.action = [](Camera &chosen_camera, f32 step)
     { chosen_camera.move({0.F, timestep * step, 0.F}); }},
}};

#ifdef VE001_USE_GLFW3
void keyCallback(GLFWwindow *window_handle, i32 key, i32, i32 action, i32)
{
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
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            light_as_camera = !light_as_camera;
        }
        break;
    case GLFW_KEY_I:
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            light_as_camera_proj = !light_as_camera_proj;
        }
        break;
    case GLFW_KEY_L:
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            light_selector = (light_selector + 1) % 3;
            light_changed = true;
        }
        break;
    }
}

void mousePosCallback(GLFWwindow *win_handle, f64 x_pos, f64 y_pos)
{
    i32 w{0};
    i32 h{0};
    glfwGetWindowSize(win_handle, &w, &h);
    const auto dw = static_cast<f64>(w);
    const auto dh = static_cast<f64>(h);
    if (x_pos == dw / 2.0 && y_pos == dh / 2.0)
    {
        return;
    }
    glfwSetCursorPos(win_handle, dw / 2.0, dh / 2.0);

    const auto aspect_ratio = static_cast<f32>(dw) / static_cast<f32>(dh);

    const auto x_step = (static_cast<f32>(x_pos) + prev_mouse_pos[0]) / 2.F - (static_cast<f32>(w) / 2.F);
    const auto y_step = aspect_ratio * ((static_cast<f32>(y_pos) + prev_mouse_pos[1]) / 2.F - (static_cast<f32>(h) / 2.F));

    static constexpr auto step{0.020F};

    auto &chosen_camera = light_as_camera ? light : camera;

    chosen_camera.rotateXYPlane({timestep * step * x_step, timestep * step * y_step});
    camera_rotated = true;

    prev_mouse_pos[0] = static_cast<f32>(x_pos);
    prev_mouse_pos[1] = static_cast<f32>(y_pos);
}
#endif


constexpr std::array<Vec3f32, 6> FACE_NORMAL_LOOKUP_TABLE {{
    { 1.F, 0.F, 0.F}, 
    {-1.F, 0.F, 0.F},
    { 0.F, 1.F, 0.F},
    { 0.F,-1.F, 0.F},
    { 0.F, 0.F, 1.F},
    { 0.F, 0.F,-1.F}
}};
u32 getCameraMask(const Camera& cam) {
    u32 mask{ 0U };    

    if (cam.looking_dir[0] > 0.F) {
        mask |= (1U << Face::X_NEG);
    } else if (cam.looking_dir[0] < 0.F) {
        mask |= (1U << Face::X_POS);
    } else {
        mask |= (3U << Face::X_NEG);
    }
    
    if (cam.looking_dir[1] > 0.F) {
        mask |= (1U << Face::Y_NEG);
    } else if (cam.looking_dir[1] < 0.F) {
        mask |= (1U << Face::Y_POS);
    } else {
        mask |= (3U << Face::Y_NEG);
    }
    
    if (cam.looking_dir[2] > 0.F) {
        mask |= (1U << Face::Z_NEG);
    } else if (cam.looking_dir[2] < 0.F) {
        mask |= (1U << Face::Z_POS);
    } else {
        mask |= (3U << Face::Z_NEG);
    }

    return mask;
}
bool backFaceCullingUnaryOp(Face orientation, vmath::Vec3f32 position, u32 camera_mask) {
    const auto normal = Vec3f32::normalize(Vec3f32::sub(position, camera.position));

    if ((camera_mask & (1U << orientation)) > 0U) {
        return true;
    }

    for (const auto corner : CORNERS) {
        const auto normal = Vec3f32::normalize(Vec3f32::sub(Vec3f32::add(position, corner), camera.position));
        if (Vec3f32::dot(normal, FACE_NORMAL_LOOKUP_TABLE[orientation]) <= .1F) {

            return true;
        }
    }
    return false;
}

struct Plane { 
    f32 A, B, C, D; 
};

std::array<Plane, 6> computeFrustumPlanes(Mat4f32 view_proj_matrix) {
    std::array<Plane, 6> result; 

    // Left plane
    result[Face::X_POS].A = view_proj_matrix[0][3] + view_proj_matrix[0][0];
    result[Face::X_POS].B = view_proj_matrix[1][3] + view_proj_matrix[1][0];
    result[Face::X_POS].C = view_proj_matrix[2][3] + view_proj_matrix[2][0];
    result[Face::X_POS].D = view_proj_matrix[3][3] + view_proj_matrix[3][0];

    // Right plane
    result[Face::X_NEG].A = view_proj_matrix[0][3] - view_proj_matrix[0][0];
    result[Face::X_NEG].B = view_proj_matrix[1][3] - view_proj_matrix[1][0];
    result[Face::X_NEG].C = view_proj_matrix[2][3] - view_proj_matrix[2][0];
    result[Face::X_NEG].D = view_proj_matrix[3][3] - view_proj_matrix[3][0];

    // Top plane
    result[Face::Y_POS].A = view_proj_matrix[0][3] - view_proj_matrix[0][1];
    result[Face::Y_POS].B = view_proj_matrix[1][3] - view_proj_matrix[1][1];
    result[Face::Y_POS].C = view_proj_matrix[2][3] - view_proj_matrix[2][1];
    result[Face::Y_POS].D = view_proj_matrix[3][3] - view_proj_matrix[3][1];

    // Bottom plane
    result[Face::Y_NEG].A = view_proj_matrix[0][3] + view_proj_matrix[0][1];
    result[Face::Y_NEG].B = view_proj_matrix[1][3] + view_proj_matrix[1][1];
    result[Face::Y_NEG].C = view_proj_matrix[2][3] + view_proj_matrix[2][1];
    result[Face::Y_NEG].D = view_proj_matrix[3][3] + view_proj_matrix[3][1];

    // Near plane
    result[Face::Z_POS].A = view_proj_matrix[0][3] + view_proj_matrix[0][2];
    result[Face::Z_POS].B = view_proj_matrix[1][3] + view_proj_matrix[1][2];
    result[Face::Z_POS].C = view_proj_matrix[2][3] + view_proj_matrix[2][2];
    result[Face::Z_POS].D = view_proj_matrix[3][3] + view_proj_matrix[3][2];

    // Far plane
    result[Face::Z_NEG].A = view_proj_matrix[0][3] - view_proj_matrix[0][2];
    result[Face::Z_NEG].B = view_proj_matrix[1][3] - view_proj_matrix[1][2];
    result[Face::Z_NEG].C = view_proj_matrix[2][3] - view_proj_matrix[2][2];
    result[Face::Z_NEG].D = view_proj_matrix[3][3] - view_proj_matrix[3][2];

    for (auto& plane : result) {
        const auto length = Vec3f32({plane.A, plane.B, plane.C}).len();
        plane.A /= length;
        plane.B /= length;
        plane.C /= length;
        plane.D /= length;
    }
    return result;
}

bool frustumCullingUnaryOp(Face orientation, vmath::Vec3f32 position, std::array<Plane, 6> planes) {
    const auto diff = Vec3f32::diff<f32>(camera.position, position);

    if (diff[0] <= EXTENT_F32[0] && diff[1] <= EXTENT_F32[1] && diff[2] <= EXTENT_F32[2]) {
        return true;
    }

    for (const auto plane : planes) {
        bool all_corners_outside{ true };
        for (const auto corner : CORNERS) {
            const auto point = Vec3f32::add(corner, position);
            const auto dist = Vec3f32::dot(point, {plane.A, plane.B, plane.C}) + plane.D;

            if (dist >= 0.F) {
                all_corners_outside = false;
                break;
            }
        }
        if (all_corners_outside) {
            return false;
        }
    }
    return true;
}

// static constexpr f32 CULLING_BIAS{ .8F };
// bool frustumAndBackFaceCullingUnaryOp(Face orientation, vmath::Vec3f32 position, u32 camera_mask, vmath::Mat4f32 view_proj_matrix) {
//     const auto normal = Vec3f32::normalize(Vec3f32::sub(position, camera.position));
//     for (const auto corner : CORNERS) {
//         const auto point = Vec3f32::add(position, corner);
//         const auto normal = Vec3f32::normalize(Vec3f32::sub(point, camera.position));
//         if (Vec3f32::dot(normal, FACE_NORMAL_LOOKUP_TABLE[orientation]) <= .2F) {
//             const auto point_in_view_space = Mat4f32::mulVec(view_proj_matrix, Vec4f32(point[0], point[1], point[2], 1.F));

//             const auto w = point_in_view_space[3] + CULLING_BIAS * point_in_view_space[3];

//             if (point_in_view_space[2] < -w || point_in_view_space[2] > w) {
//                 continue;
//             }

//             if (point_in_view_space[0] < -w || point_in_view_space[0] > w ||
//                 point_in_view_space[1] < -w || point_in_view_space[1] > w) {
//                 continue;
//             }   

//             return true;
//         }
//     }
//     return false;
// }

int main()
{
    if (!ve001::window.init("demo", 640, 480, nullptr))
    {
        return 1;
    }

    ve001::glInit();
    // ve001::setGLDebugCallback([](u32, u32, u32 id, u32, int, const char *message, const void *) {
    // 	fmt::print("[GLAD]{}:{}", id, message);
    // });

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Engine engine({500.F, 200.F, 500.F}, {0.F, 0.F, 0.F}, EXTENT);
    // Engine engine({300.F, 210.F, 300.F}, {0.F, 0.F, 0.F}, EXTENT);
    // Engine engine({100.F, 80.F, 100.F}, {0.F, 0.F, 0.F}, EXTENT);
    //Engine engine({64.F, 64.F, 64.F}, {0.F, 0.F, 0.F}, EXTENT);
    engine.init();

    MaterialAllocator material_allocator(2U, 5U, 12U, 96U, 96U);

    material_allocator.init();

    material_allocator.addMaterialParams(MaterialParams{
        .specular = Vec4f32(1.F, 1.F, 1.F, 128.F),
        .diffuse = Vec3f32(1.F),
        .alpha = 1.F});

    createCubeMapTextureArray(
        material_allocator,
        "/home/regu/codium_repos/VE-001/assets/textures/mcgrasstexture_small.png"
    );
    createCubeMapTextureArray(
        material_allocator,
        "/home/regu/codium_repos/VE-001/assets/textures/mcstonetexture_small.png"
    );

    material_allocator.addMaterial(MaterialDescriptor{
        .material_texture_index = {0U, 1U, 2U, 3U, 4U, 5U},
        .material_params_index = {0U, 0U, 0U, 0U, 0U, 0U}});
    material_allocator.addMaterial(MaterialDescriptor{
        .material_texture_index = {6U, 7U, 8U, 9U, 10U, 11U},
        .material_params_index = {0U, 0U, 0U, 0U, 0U, 0U}});

    material_allocator._texture_rgba8_array.bind(VE001_SH_CONFIG_TEX_BINDING_MATERIAL_TEXTURES);
    material_allocator._material_params_array.bind(VE001_SH_CONFIG_SSBO_BINDING_MATERIAL_PARAMS);
    material_allocator._material_descriptors_array.bind(VE001_SH_CONFIG_SSBO_BINDING_MATERIAL_DESCRIPTORS);

#ifdef VE001_USE_GLFW3
    window.setKeyCallback(keyCallback);
    window.setMousePositionCallback(mousePosCallback);
#endif

    glClearColor(.22F, .61F, .78F, 1.F);

    struct General
    {
        Mat4f32 mvp;
        alignas(16) Vec3f32 camera_pos;
        alignas(16) Vec3f32 camera_dir;
    } general;

    GPUBuffer general_ubo(sizeof(General));
    general_ubo.init();
    general_ubo.bind(GL_UNIFORM_BUFFER, VE001_SH_CONFIG_UBO_BINDING_GENERAL);

    glEnable(GL_FRAMEBUFFER_SRGB);

    Lighting lighting(1024, 1024);
    lighting.initNoShaders();

    lighting._shadow_map_point_shader = engine._engine_context.shader_repo[ShaderType::SHADOW_MAP_POINT_SHADER];
    lighting._shadow_map_dir_shader = engine._engine_context.shader_repo[ShaderType::SHADOW_MAP_DIR_SHADER];
    lighting._shadow_map_spot_shader = engine._engine_context.shader_repo[ShaderType::SHADOW_MAP_SPOT_SHADER];

    lighting._directional._buffer.bind(VE001_SH_CONFIG_SSBO_BINDING_DIRECTIONAL_LIGHTS);
    lighting._point._buffer.bind(VE001_SH_CONFIG_SSBO_BINDING_POINT_LIGHTS);
    lighting._spot._buffer.bind(VE001_SH_CONFIG_SSBO_BINDING_SPOT_LIGHTS);
    lighting._descriptor_buffer.bind(GL_UNIFORM_BUFFER, VE001_SH_CONFIG_UBO_BINDING_LIGHTS_DESCRIPTOR);
    lighting._shadow_map_array.bind(VE001_SH_CONFIG_TEX_BINDING_DIR_SHADOW_MAPS);
    lighting._cube_shadow_map_array.bind(VE001_SH_CONFIG_TEX_BINDING_OMNI_DIR_SHADOW_MAPS);

    const auto point_light_id = lighting.addPointLight({.params = {
                                                            .position = Vec3f32(0.F, 0.F, 0.F),
                                                            .ambient = Vec3f32(.05F),
                                                            .diffuse = Vec3f32(.2F, .1F, .93F),
                                                            .specular = Vec3f32(.2F, .1F, .93F),
                                                            .attenuation_params = Vec3f32(1.F, .0009F, .0032F)},
                                                        .shadow_casting = true});
    const auto spot_light_id = lighting.addSpotLight({.params = {
                                                          .position = Vec3f32(0.F, 0.F, 0.F),
                                                          .direction = Vec3f32(0.F, 0.F, 1.F),
                                                          .ambient = Vec3f32(.2F, .1F, .1F),
                                                          .diffuse = Vec3f32(.99F, .42F, .42F),
                                                          .specular = Vec3f32(1.F, .42F, .42F),
                                                          .attenuation_params = Vec3f32(2.F, .009F, .0032F),
                                                          .cut_off = Vec3f32(
                                                              std::cos(std::numbers::pi_v<f32> / 8.F),
                                                              std::cos(std::numbers::pi_v<f32> / 6.4F),
                                                              std::cos(std::numbers::pi_v<f32> / 8.F) - std::cos(std::numbers::pi_v<f32> / 6.4F))},
                                                      .shadow_casting = true});
    const auto dir_light_id = lighting.addDirectionalLight({.params = {
                                                                .direction = Vec3f32(0.F, 0.F, -1.F),
                                                                .ambient = Vec3f32(.05F),
                                                                .diffuse = Vec3f32(.59F, .48F, .42F),
                                                                .specular = Vec3f32(1.F, 1.F, 1.F),
                                                            },
                                                            .shadow_casting = true});

    struct LightBobo
    {
        u32 id;
        bool perspective;
    };

    const std::array<LightBobo, 3> lights{{
        {point_light_id, true},
        {spot_light_id, true},
        {dir_light_id, false},
    }};

    // light.move({-20.F, 40.F, 20.F});
    camera.move({0.F, 0.F, 0.F});

    const auto render_scene = [&chunk_pool = engine._world_grid._chunk_pool]()
    { chunk_pool.drawAll(true); };

    f32 prev_frame_time{0.F};
    while (!window.shouldClose())
    {
        const auto [window_width, window_height] = window.size();

        const auto frame_time = window.time();
        timestep = frame_time - prev_frame_time;
        prev_frame_time = frame_time;

        for (auto &key : keys)
        {
            if (key.pressed)
            {
                key.action(light_as_camera ? light : camera, 20.F);
                camera_moved = true;
            }
        }

        auto &light_bobo = lights[light_selector];

        if (light_changed)
        {
            light.position = lighting.getLightPosition(light_bobo.id);
            light.looking_dir_angles = lighting.getLightRotation(light_bobo.id);
            light_changed = false;
        }

        const auto proj_mat = misc<f32>::symmetricPerspectiveProjection(
            .4F * std::numbers::pi_v<f32>, .1F, 1000.F,
            static_cast<f32>(window_width), static_cast<f32>(window_height));

        const auto mvp = Mat4f32::mul(proj_mat, camera.lookAt());

        const auto light_proj_mat = light_bobo.perspective ? misc<f32>::symmetricPerspectiveProjection(
                                                                 .5F * std::numbers::pi_v<f32> /*1.41F * std::numbers::pi_v<f32>/7.4F*/, .1F, 100.F,
                                                                 static_cast<f32>(window_width), static_cast<f32>(window_height))
                                                           : misc<f32>::symmetricOrthographicProjection(
                                                                 .1F, 1000.F,
                                                                 static_cast<f32>(window_width), static_cast<f32>(window_height));

        general.mvp = light_as_camera_proj ? Mat4f32::mul(light_proj_mat, light.lookAt()) : mvp;
        general.camera_pos = camera.position;
        general.camera_dir = camera.looking_dir;
        // .light_dir = -light.looking_dir

        general_ubo.write(static_cast<const void *>(&general));

        // if (light_as_camera_proj)
        // {
        //     static bool done = false;
        //     if (!done)
        //     {
        //         // terrain_generator.next(static_cast<void *>(noise.data()), 0U, 0U, {2, 0, 2});
        //         const auto chunk_id = engine._chunk_pool.allocateChunk(std::span<u16>(noise), {0, 0, 0});
        //         done = true;
        //     }
        // }

        engine._world_grid.update(camera.position);
        
        // engine._world_grid._chunk_pool.poll();

        if (engine._world_grid._chunk_pool.poll()) {
        //     light.position = lighting.getLightPosition(light_bobo.id);
        //     light.looking_dir_angles = lighting.getLightRotation(light_bobo.id);
        //     engine._world_grid._chunk_pool.partitionDrawCommands(backFaceCullingUnaryOp, false, getCameraMask(light));
        //     engine._world_grid._chunk_pool.partitionDrawCommands<const std::array<Plane, 6>&>(frustumCullingUnaryOp, true, computeFrustumPlanes(Mat4f32::mul(light_proj_mat, light.lookAt())));
        //     engine._world_grid._chunk_pool.update(true);//!light_as_camera);
            // lighting.moveLight(dir_light_id, camera.position);
            lighting.rotateLight(dir_light_id, light.looking_dir);
            // light.position = camera.position;
        //     engine._world_grid._chunk_pool.forceCommandsDirty();
        }

        if ((camera_moved || camera_rotated) && !light_as_camera) {
            // lighting.moveLight(dir_light_id, camera.position);
            // lighting.rotateLight(dir_light_id, light.looking_dir);
            // light.position = camera.position;
            // engine._world_grid._chunk_pool.partitionDrawCommands(backFaceCullingUnaryOp, false, getCameraMask(camera));
            engine._world_grid._chunk_pool.partitionDrawCommands(frustumCullingUnaryOp, false, computeFrustumPlanes(
                Mat4f32::mul(misc<f32>::symmetricPerspectiveProjection(
                    .6F * std::numbers::pi_v<f32>, .1F, 1000.F,
                    static_cast<f32>(window_width), static_cast<f32>(window_height))
                , camera.lookAt())
            ));
            camera_moved = false;
            camera_rotated = false;
        }

        engine._world_grid._chunk_pool.update(true);//!light_as_camera);

        if (camera_rotated && light_as_camera)
        {
            const auto dir_light_rot_angles = lighting.getLightRotation(light_bobo.id);
            lighting.rotateLight(light_bobo.id, light.looking_dir); // Vec3f32::sub(light.looking_dir_angles, dir_light_rot_angles));
            camera_rotated = false;
        }
        if (camera_moved && light_as_camera)
        {
            lighting.moveLight(light_bobo.id, light.position);
            camera_moved = false;
        }

        // render shadow map
        if (lighting.update(render_scene))
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glTextureBarrier(); // barrier since depth texture is used in subsequent draw
        }
        // main render
        glViewport(0, 0, window_width, window_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        engine._engine_context.shader_repo[ShaderType::MULTI_LIGHTS_SHADER].bind();
        render_scene();
        // engine._world_grid._chunk_pool.drawAll(false);

        // if (static int i = 0; !light_as_camera && i == 80) {
        //     logger->info("Num of draw commands: {}", engine._world_grid._chunk_pool._draw_cmds_parition_size);
        //     i = 0;
        // } else if (i == 80) {
        //     logger->info("Num of draw commands: {}", engine._world_grid._chunk_pool._draw_cmds.size());
        //     i = 0;
        // } else {
        //     ++i;
        // }


        window.swapBuffers();
        window.pollEvents();
    }

    logger->info("Number of triangles was :: {:L}", 
        engine._world_grid._chunk_pool.gpu_memory_usage / (6 * sizeof(Vertex))
    );
    logger->info("GPU memory usage was :: {}B, {}kB, {}MB", 
        engine._world_grid._chunk_pool.gpu_memory_usage,
        engine._world_grid._chunk_pool.gpu_memory_usage / 1024,
        engine._world_grid._chunk_pool.gpu_memory_usage / (1024 * 1024)
    );
    logger->info("CPU memory usage was :: {}B, {}kB, {}MB", 
        engine._world_grid._chunk_pool.cpu_memory_usage,
        engine._world_grid._chunk_pool.cpu_memory_usage / 1024,
        engine._world_grid._chunk_pool.cpu_memory_usage / (1024 * 1024)
    );

    general_ubo.deinit();
    lighting.deinitNoShaders();
    material_allocator.deinit();
    engine.deinit();
    window.deinit();

    return 0;
}
