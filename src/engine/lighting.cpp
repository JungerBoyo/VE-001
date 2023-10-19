#include "lighting.h"
#include "enums.h"

#include <glad/glad.h>

#include <cstring>

using namespace ve001;
using namespace vmath;

static constexpr Mat4f32 DEPTH_BIAS_MATRIX {
    Vec4f32{ .5F, 0.F, 0.F, 0.F},
    Vec4f32{ 0.F, .5F, 0.F, 0.F},
    Vec4f32{ 0.F, 0.F, .5F, 0.F},
    Vec4f32{ .5F, .5F, .5F, 1.F}
};

// static constexpr std::array<Vec3f32, 6> POINT_LIGHT_VECTORS {{
//     Vec3f32( 1.F, 0.F, 0.F),
//     Vec3f32(-1.F, 0.F, 0.F),
//     Vec3f32( 0.F, 1.F, 0.F),
//     Vec3f32( 0.F,-1.F, 0.F),
//     Vec3f32( 0.F, 0.F, 1.F),
//     Vec3f32( 0.F, 0.F,-1.F)
// }};
    
enum LightMask : vmath::u32 {
    DIRECTIONAL = 0x80000000U,
    POINT       = 0x40000000U,
    SPOT        = 0xC0000000U,
    ALL         = 0xC0000000U
};

static void setAsComparable(u32 tex_id) {
    glTextureParameteri(tex_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(tex_id, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTextureParameteri(tex_id, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
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
    _cube_shadow_map_array(shadow_map_width, shadow_map_height, 1, SHADOW_MAP_TEXTURE_PARAMS)
{
    _descriptor.shadow_map_width = static_cast<f32>(shadow_map_width);
    _descriptor.shadow_map_height = static_cast<f32>(shadow_map_height);
    _descriptor.shadow_map_aspect_ratio = _descriptor.shadow_map_width/_descriptor.shadow_map_height;  
    _descriptor.cube_shadow_map_n = (50.F + 1.F) / (50.F - 1.F) * .5F + .5F;
    _descriptor.cube_shadow_map_f =-(50.F * 1.F) / (50.F - 1.F);
}

void Lighting::init() {
    _directional.init(1);
    _point.init(1);
    _spot.init(1);

    _descriptor_buffer.init();

    _shadow_map_array.init();
    _cube_shadow_map_array.init();

    _shadow_mapping_fbo.initRaw();
    _shadow_mapping_fbo.setDrawBuffer(GL_NONE);
    _shadow_mapping_fbo.setReadBuffer(GL_NONE);

    _shadow_map_dir_shader.init();
    _shadow_map_dir_shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_dir_shader/vert.spv",
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_dir_shader/frag.spv",
        true
    );
    _shadow_map_spot_shader.init();
    _shadow_map_spot_shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_spot_shader/vert.spv",
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_spot_shader/frag.spv",
        true
    );
    _shadow_map_point_shader.init();
    _shadow_map_point_shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_point_shader/vert.spv",
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_point_shader/geom.spv",
        "/home/regu/codium_repos/VE-001/shaders/bin/shadow_map_point_shader/frag.spv",
        true
    );
}

void Lighting::initNoShaders() {
    _directional.init(1);
    _point.init(1);
    _spot.init(1);

    _descriptor_buffer.init();

    _shadow_map_array.init();
    _cube_shadow_map_array.init();

    _shadow_mapping_fbo.initRaw();
    _shadow_mapping_fbo.setDrawBuffer(GL_NONE);
    _shadow_mapping_fbo.setReadBuffer(GL_NONE);

    _descriptor_buffer.write(&_descriptor);
}

bool Lighting::update(const std::function<void()>& render_scene) {
    const auto result = (
        !_dirty_dir_light_ids.empty()   ||
        !_dirty_point_light_ids.empty() ||
        !_dirty_spot_light_ids.empty()
    );

    if (result) {
        glViewport(
            0, 0, 
            static_cast<GLsizei>(_descriptor.shadow_map_width),
            static_cast<GLsizei>(_descriptor.shadow_map_height) 
        );
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.F, 4.F);
    } else {
        return false;
    }

    if (!_dirty_dir_light_ids.empty()) {
        _shadow_map_dir_shader.bind();
        for (const auto light_id : _dirty_dir_light_ids) {
            auto& light = _directional.get(light_id & ~LightMask::DIRECTIONAL);
            if (light.shadow_casting) {
                light.params.transform = Mat4f32::mul(light.projection, light.camera.lookAt());
                _descriptor_buffer.write(static_cast<const void*>(&light.params.transform), 0U, sizeof(Mat4f32));

                light.params.transform = Mat4f32::mul(DEPTH_BIAS_MATRIX, light.params.transform);
                _directional.update(light_id & ~LightMask::DIRECTIONAL);

                _shadow_mapping_fbo.bind();
                _shadow_mapping_fbo.bindTexLayerToAttachment(
                    GL_DEPTH_ATTACHMENT,
                    _shadow_map_array._tex_id,
                    static_cast<i32>(light.params.shadow_map_index)
                );
                glClear(GL_DEPTH_BUFFER_BIT);

                render_scene();
            } else {
                _directional.update(light_id & ~LightMask::DIRECTIONAL);
            }
        }
        _dirty_dir_light_ids.clear();
    }
    if (!_dirty_point_light_ids.empty()) {
        _shadow_map_point_shader.bind();
        for (const auto light_id : _dirty_point_light_ids) {
            auto& light = _point.get(light_id & ~LightMask::POINT);
            if (light.shadow_casting) {
                const auto light_pos = light.camera.position;
                const auto proj = misc<f32>::symmetricPerspectiveProjection(std::numbers::pi_v<f32>/2.F, 1.F, 50.F, 1024.F, 1024.F);
                std::array<Mat4f32, 6> transforms;
                transforms[0] = Mat4f32::mul(proj, misc<f32>::lookAt(light_pos, Vec3f32( 1.F, 0.F, 0.F), Vec3f32(0.F,-1.F, 0.F)));
                transforms[1] = Mat4f32::mul(proj, misc<f32>::lookAt(light_pos, Vec3f32(-1.F, 0.F, 0.F), Vec3f32(0.F,-1.F, 0.F)));
                transforms[2] = Mat4f32::mul(proj, misc<f32>::lookAt(light_pos, Vec3f32( 0.F, 1.F, 0.F), Vec3f32(0.F, 0.F, 1.F)));
                transforms[3] = Mat4f32::mul(proj, misc<f32>::lookAt(light_pos, Vec3f32( 0.F,-1.F, 0.F), Vec3f32(0.F, 0.F,-1.F)));
                transforms[4] = Mat4f32::mul(proj, misc<f32>::lookAt(light_pos, Vec3f32( 0.F, 0.F, 1.F), Vec3f32(0.F,-1.F, 0.F)));
                transforms[5] = Mat4f32::mul(proj, misc<f32>::lookAt(light_pos, Vec3f32( 0.F, 0.F,-1.F), Vec3f32(0.F,-1.F, 0.F)));

                _descriptor_buffer.write(static_cast<const void*>(transforms.data()), 0U, transforms.size() * sizeof(Mat4f32));
                
                _point.update(light_id & ~LightMask::POINT);
                
                _shadow_mapping_fbo.bind();
                _shadow_mapping_fbo.bindTexToAttachment(
                    GL_DEPTH_ATTACHMENT,
                    _cube_shadow_map_array._tex_id
                );
                glClear(GL_DEPTH_BUFFER_BIT);

                render_scene();
            } else {
                _point.update(light_id & ~LightMask::POINT);
            }
        }
        _dirty_point_light_ids.clear();
    }
    if (!_dirty_spot_light_ids.empty()) {
        _shadow_map_spot_shader.bind();
        for (const auto light_id : _dirty_spot_light_ids) {
            auto& light = _spot.get(light_id & ~LightMask::SPOT);
            if (light.shadow_casting) {
                light.params.transform = Mat4f32::mul(light.projection, light.camera.lookAt());
                _descriptor_buffer.write(static_cast<const void*>(&light.params.transform), 0U, sizeof(Mat4f32));

                light.params.transform = Mat4f32::mul(DEPTH_BIAS_MATRIX, light.params.transform);
                _spot.update(light_id & ~LightMask::SPOT);
                
                _shadow_mapping_fbo.bindTexLayerToAttachment(
                    GL_DEPTH_ATTACHMENT,
                    _shadow_map_array._tex_id,
                    static_cast<i32>(light.params.shadow_map_index)
                );
                _shadow_mapping_fbo.bind();
                glClear(GL_DEPTH_BUFFER_BIT);

                render_scene();
            } else {
                _spot.update(light_id & ~LightMask::SPOT);
            }
        }
        _dirty_spot_light_ids.clear();
    }
    
    if (result) {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    return result;
}

u32 Lighting::addDirectionalLight(DirectionalLight dir_light) {
    dir_light.projection = misc<f32>::symmetricOrthographicProjection(
        .1F, 1000.F, 
        _descriptor.shadow_map_width, 
        _descriptor.shadow_map_height
    );
    if (dir_light.shadow_casting) {
        const auto layer = _shadow_map_array.size();
        _shadow_map_array.emplace();
        dir_light.params.shadow_map_index = static_cast<f32>(layer);
    }
    const auto id = _directional.add(dir_light) | LightMask::DIRECTIONAL;
    ++_descriptor.directional_lights_size;
    _dirty_dir_light_ids.push_back(id);
    
    _descriptor_buffer.write(
        &_descriptor.directional_lights_size,
        offsetof(Descriptor, directional_lights_size),
        sizeof(Descriptor::directional_lights_size)
    );

    return id;
}
u32 Lighting::addPointLight(PointLight point_light) {
    point_light.projection = misc<f32>::symmetricPerspectiveProjection(
        std::numbers::pi_v<f32> / 2.F, 1.F, 50.F, 
        _descriptor.shadow_map_width, 
        _descriptor.shadow_map_height
    );
    if (point_light.shadow_casting) {
        const auto layer = _cube_shadow_map_array.size();
        _cube_shadow_map_array.emplace();
        point_light.params.shadow_map_index = static_cast<f32>(layer);
    }
    const auto id = _point.add(point_light) | LightMask::POINT;
    ++_descriptor.point_lights_size;
    _dirty_point_light_ids.push_back(id);

    _descriptor_buffer.write(
        &_descriptor.point_lights_size,
        offsetof(Descriptor, point_lights_size),
        sizeof(Descriptor::point_lights_size)
    );
    
    return id;
}
u32 Lighting::addSpotLight(SpotLight spot_light) {
    spot_light.projection = misc<f32>::symmetricPerspectiveProjection(
        .5F * std::numbers::pi_v<f32>, .1F, 100.F,
        // 1.41F * std::acos(spot_light.params.cut_off[1]), .1F, 100.F, 
        _descriptor.shadow_map_width,
        _descriptor.shadow_map_height
    );
    if (spot_light.shadow_casting) {
        const auto layer = _shadow_map_array.size();
        _shadow_map_array.emplace();
        spot_light.params.shadow_map_index = static_cast<f32>(layer);
    }
    const auto id = _spot.add(spot_light) | LightMask::SPOT;
    ++_descriptor.spot_lights_size;
    _dirty_spot_light_ids.push_back(id);

    _descriptor_buffer.write(
        &_descriptor.spot_lights_size,
        offsetof(Descriptor, spot_lights_size),
        sizeof(Descriptor::spot_lights_size)
    );
    
    return id;
}

void Lighting::moveLight(u32 light_id, Vec3f32 move_vec) {
    switch (light_id & LightMask::ALL) {
    case LightMask::DIRECTIONAL: /* directional light can't move */ break;
    case LightMask::POINT: {
        const auto id = (light_id & ~LightMask::POINT);
        auto& light = _point.get(id);
        light.camera.position = move_vec;
        // light.camera.move(move_vec);
        light.params.position = light.camera.position;
        _dirty_point_light_ids.push_back(light_id);
        break;
    }
    case LightMask::SPOT: {
        const auto id = (light_id & ~LightMask::SPOT);
        auto& light = _spot.get(id);
        light.camera.position = move_vec;
        // light.camera.move(move_vec);
        light.params.position = light.camera.position;
        _dirty_spot_light_ids.push_back(light_id);
        break;
    }
    }
}
void Lighting::rotateLight(u32 light_id, Vec3f32 angles) {
    switch (light_id & LightMask::ALL) {
    case LightMask::DIRECTIONAL: {
        const auto id = (light_id & ~LightMask::DIRECTIONAL);
        auto& light = _directional.get(id);
        // light.camera.rotate(angles);
        light.params.direction = -angles;
        light.camera.looking_dir = angles;
        // light.params.direction = -light.camera.looking_dir;
        _dirty_dir_light_ids.push_back(light_id);
        break;
    }
    case LightMask::POINT: /* spot light can't rotate */ break;
    case LightMask::SPOT: {
        const auto id = (light_id & ~LightMask::SPOT);
        auto& light = _spot.get(id);
        // light.camera.rotate(angles);
        light.params.direction = -angles;
        light.camera.looking_dir = angles;
        // light.params.direction = -light.camera.looking_dir;
        _dirty_spot_light_ids.push_back(light_id);
        break;
    }
    }
}

Vec3f32 Lighting::getLightPosition(vmath::u32 light_id) {
    switch (light_id & LightMask::ALL) {
    case LightMask::DIRECTIONAL: return Vec3f32{};
    case LightMask::POINT: {
        const auto id = (light_id & ~LightMask::POINT);
        auto& light = _point.get(id);
        return light.params.position;
    }
    case LightMask::SPOT: {
        const auto id = (light_id & ~LightMask::SPOT);
        auto& light = _spot.get(id);
        return light.params.position;
    }
    }
    return Vec3f32{};
}
Vec3f32 Lighting::getLightRotation(vmath::u32 light_id) {
    switch (light_id & LightMask::ALL) {
    case LightMask::DIRECTIONAL: {
        const auto id = (light_id & ~LightMask::DIRECTIONAL);
        auto& light = _directional.get(id);
        return light.camera.looking_dir_angles;
    }
    case LightMask::POINT: return Vec3f32{};
    case LightMask::SPOT: {
        const auto id = (light_id & ~LightMask::SPOT);
        auto& light = _spot.get(id);
        return light.camera.looking_dir_angles;
    }
    }
    return Vec3f32{};
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

void Lighting::deinitNoShaders() {
    _directional.deinit();
    _point.deinit();
    _spot.deinit();
    _descriptor_buffer.deinit();
    _shadow_map_array.deinit();
    _cube_shadow_map_array.deinit();
    _shadow_mapping_fbo.deinit();
}
