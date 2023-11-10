#ifndef VE001_LIGHTING_H
#define VE001_LIGHTING_H

#include <vmath/vmath_types.h>
#include "light.h"
#include "gpu_object_pooling_resource.h"
#include "gpu_buffer.h"
#include "texture_array.h"
#include "cubetexture_array.h"
#include "shader.h"
#include "framebuffer.h"

#include <vector>
#include <functional>

namespace ve001 {

struct Lighting {
    struct Descriptor {
        vmath::Mat4f32 current_light_transform[6];
        vmath::f32 shadow_map_width;
        vmath::f32 shadow_map_height;
        vmath::f32 shadow_map_aspect_ratio;
        vmath::f32 cube_shadow_map_n;
        vmath::f32 cube_shadow_map_f;
        vmath::u32 directional_lights_size{ 0U };
        vmath::u32 point_lights_size{ 0U };
        vmath::u32 spot_lights_size{ 0U };
    };
    GPUObjectPoolingResource<DirectionalLight> _directional;
    GPUObjectPoolingResource<PointLight> _point;
    GPUObjectPoolingResource<SpotLight> _spot;
    Descriptor _descriptor;
    GPUBuffer _descriptor_buffer{sizeof(Descriptor)};
    TextureArray _shadow_map_array;
    CubeTextureArray _cube_shadow_map_array;
    Framebuffer _shadow_mapping_fbo;
    Shader _shadow_map_dir_shader;
    Shader _shadow_map_point_shader;
    Shader _shadow_map_spot_shader;
    std::vector<vmath::u32> _dirty_dir_light_ids;
    std::vector<vmath::u32> _dirty_point_light_ids;
    std::vector<vmath::u32> _dirty_spot_light_ids;

    Lighting(vmath::i32 shadow_map_width, vmath::i32 shadow_map_height);
    
    void init();
    void initNoShaders();

    vmath::u32 addDirectionalLight(DirectionalLight dir_light);
    vmath::u32 addPointLight(PointLight point_light);
    vmath::u32 addSpotLight(SpotLight spot_light);

    void moveLight(vmath::u32 light_id, vmath::Vec3f32 move_vec);
    void rotateLight(vmath::u32 light_id, vmath::Vec3f32 angles);

    vmath::Vec3f32 getLightPosition(vmath::u32 light_id);
    vmath::Vec3f32 getLightRotation(vmath::u32 light_id);

    bool update(const std::function<void()>& render_scene);

    /*
        tam shader'y.
        Errory zunifikowac ogarnąć temat
        renderowanie do shadow mapy
        framebuffer shadowmapowy tutaj
        okokok 
    */

    void deinit();
    void deinitNoShaders();

}; 

}

#endif