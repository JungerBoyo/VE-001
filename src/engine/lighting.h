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

namespace ve001 {

struct Lighting {
    struct Descriptor {
        vmath::Mat4f32 current_light_transform[6];
        alignas(16) vmath::f32 shadow_map_width;
        alignas(16) vmath::f32 shadow_map_height;
        alignas(16) vmath::f32 shadow_map_aspect_ratio;
        alignas(16) vmath::u32 directional_lights_size{ 0U };
        alignas(16) vmath::u32 point_lights_size{ 0U };
        alignas(16) vmath::u32 spot_lights_size{ 0U };
    };
    GPUObjectPoolingResource<DirectionalLight> _directional;
    GPUObjectPoolingResource<PointLight> _point;
    GPUObjectPoolingResource<SpotLight> _spot;
    Descriptor _descriptor;
    GPUBuffer _descriptor_buffer{sizeof(Descriptor)};
    TextureArray _shadow_map_array;
    CubeTextureArray _cube_shadow_map_array;
    i32 _shadow_map_width;
    i32 _shadow_map_height;
    Framebuffer _shadow_mapping_fbo;
    Shader _shadow_map_dir_shader;
    Shader _shadow_map_point_shader;
    Shader _shadow_map_spot_shader;

    std::vector<vmath::u32> _dirty_lights;

    Lighting(vmath::i32 shadow_map_width, vmath::i32 shadow_map_height);
    
    void init();

    vmath::u32 addDirectionalLight(DirectionalLight dir_light);
    vmath::u32 addPointLight(PointLight point_light);
    vmath::u32 addSpotLight(SpotLight spot_light);

    void moveLight(vmath::u32 light_id, Vec3f32 move_vec);
    void rotateLight(vmath::u32 light_id, Vec3f32 angles);

    void update();

    /*
        tam shader'y.
        Errory zunifikowac ogarnąć temat
        renderowanie do shadow mapy
        framebuffer shadowmapowy tutaj
        okokok 
    */

    void deinit();

}; 

}

#endif