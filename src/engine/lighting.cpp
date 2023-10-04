#include "lighting.h"

#include <glad/glad.h>

using namespace ve001;
using namespace vmath;

enum LightMask : vmath::u32 {
    DIRECTIONAL = 0x80000000U,
    POINT       = 0x40000000U,
    SPOT        = 0xC0000000U,
    ALL         = 0xC0000000U
};

static void setAsComparable(u32 tex_id) {
    glTextureParameteri(tex_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(tex_id, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
}

static constexpr TextureParams SHADOW_MAP_TEXTURE_PARAMS = {
    .internal_format = GL_DEPTH_COMPONENT32F,
    .format = GL_DEPTH_COMPONENT,
    .type = GL_FLOAT,
    .wrap_s = GL_CLAMP_TO_EDGE,
    .wrap_t = GL_CLAMP_TO_EDGE,
    .min_filter = GL_LINEAR,//GL_NEAREST,
    .mag_filter = GL_LINEAR,//GL_NEAREST,
    .gen_mip_map = false,
    .set_aux_params = setAsComparable
};


Lighting::Lighting(i32 shadow_map_width, i32 shadow_map_height) : 
    _shadow_map_array(shadow_map_width, shadow_map_height, 2, SHADOW_MAP_TEXTURE_PARAMS),
    _cube_shadow_map_array(shadow_map_width, shadow_map_height, 1, SHADOW_MAP_TEXTURE_PARAMS),
    _shadow_map_width(shadow_map_width),
    _shadow_map_height(shadow_map_height)
{}

void Lighting::init() {
    _directional.init(1);
    _point.init(1);
    _spot.init(1);
    _descriptor_buffer.init();
    _shadow_map_array.init();
    _cube_shadow_map_array.init();
    _shadow_mapping_fbo.initRaw();
    _shadow_map_dir_shader.init();
    _shadow_map_dir_shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_dir_shader/vert.spv",
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_dir_shader/frag.spv"
    );
    _shadow_map_spot_shader.init();
    _shadow_map_spot_shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_spot_shader/vert.spv",
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_spot_shader/frag.spv"
    );
    _shadow_map_point_shader.init();
    _shadow_map_point_shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_point_shader/vert.spv",
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_point_shader/geom.spv",
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_point_shader/frag.spv"
    );
}

void Lighting::update() {

}

u32 Lighting::addDirectionalLight(DirectionalLight dir_light) {
    dir_light.projection = misc<f32>::symmetricOrthographicProjection(.1F, 1000.F, _shadow_map_width, _shadow_map_height);
    if (dir_light.shadow_casting) {
        const auto layer = _shadow_map_array.size();
        _shadow_map_array.emplace();
        dir_light.params.shadow_map_index = static_cast<f32>(layer);
    }
    const auto id = _directional.add(dir_light) | LightMask::DIRECTIONAL;
    ++_descriptor.directional_lights_size;
    _dirty_lights.push_back(id);

    return id;
}
u32 Lighting::addPointLight(PointLight point_light) {
    point_light.projection = misc<f32>::symmetricPerspectiveProjection(
        std::numbers::pi_v<f32>/2.F, .1F, 1000.F, 
        static_cast<f32>(_shadow_map_width), 
        static_cast<f32>(_shadow_map_height)
    );
    if (point_light.shadow_casting) {
        const auto layer = _cube_shadow_map_array.size();
        _cube_shadow_map_array.emplace();
        point_light.params.shadow_map_index = static_cast<f32>(layer);
    }
    const auto id = _point.add(point_light) | LightMask::POINT;
    ++_descriptor.point_lights_size;
    _dirty_lights.push_back(id);
    return id;
}
u32 Lighting::addSpotLight(SpotLight spot_light) {
    spot_light.projection = misc<f32>::symmetricPerspectiveProjection(
        std::acos(spot_light.params.cut_off[0]), .1F, 1000.F, 
        static_cast<f32>(_shadow_map_width), 
        static_cast<f32>(_shadow_map_height)
    );
    if (spot_light.shadow_casting) {
        const auto layer = _shadow_map_array.size();
        _shadow_map_array.emplace();
        spot_light.params.shadow_map_index = static_cast<f32>(layer);
    }
    const auto id = _spot.add(spot_light) | LightMask::SPOT;
    ++_descriptor.spot_lights_size;
    _dirty_lights.push_back(id);
    return id;
}

void Lighting::moveLight(u32 light_id, Vec3f32 move_vec) {
    switch (light_id & LightMask::ALL) {
    case LightMask::DIRECTIONAL: /* directional light can't move */ break;
    case LightMask::POINT: 
        const auto id = (light_id & ~LightMask::POINT);
        auto& light = _point.get(id);
        light.camera.move(move_vec);
        _dirty_lights.push_back(light_id);
    break;
    case LightMask::SPOT: 
        const auto id = (light_id & ~LightMask::SPOT);
        auto& light = _spot.get(id);
        light.camera.move(move_vec);
        _dirty_lights.push_back(light_id);
    break;
    }
}
void Lighting::rotateLight(u32 light_id, Vec3f32 angles) {
    switch (light_id & LightMask::ALL) {
    case LightMask::DIRECTIONAL: 
        const auto id = (light_id & ~LightMask::DIRECTIONAL);
        auto& light = _directional.get(id);
        light.camera.rotate(angles);
        _dirty_lights.push_back(light_id);
    break;
    case LightMask::POINT: /* spot light can't rotate */ break;
    case LightMask::SPOT: 
        const auto id = (light_id & ~LightMask::SPOT);
        auto& light = _spot.get(id);
        light.camera.move(angles);
        _dirty_lights.push_back(light_id);
    break;
    }
}

void Lighting::deinit() {
    _directional.deinit();
    _point.deinit();
    _spot.deinit();
    _descriptor_buffer.deinit();
    _shadow_map_array.deinit();
    _cube_shadow_map_array.deinit();
    _shadow_mapping_fbo.deinit();
    _shadow_map_dir_shader.deinit();
    _shadow_map_point_shader.deinit();
    _shadow_map_spot_shader.deinit();
}