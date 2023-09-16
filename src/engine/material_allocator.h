#ifndef VE001_MATERIAL_ALLOCATOR_H
#define VE001_MATERIAL_ALLOCATOR_H

#include "vmath/vmath.h"
#include "texture_array.h"
#include "gpu_dynamic_array.h"

using namespace vmath;

namespace ve001 {

struct MaterialParams {
    Vec4f32 color;
    Vec3f32 diffuse;
    f32 shininess;
    Vec3f32 specular;
};

struct MaterialDescriptor {
    u32 material_texture_index[6];
    u32 material_params_index[6];
};

// TODO: handle the case material deleation? 
// TODO2: multiple texture arrays?
struct MaterialAllocator {
    TextureArray _texture_rgba8_array;
    GPUDynamicArray _material_params_array;
    GPUDynamicArray _material_descriptors_array;

    MaterialAllocator(
        u32 material_params_capacity, 
        u32 material_descriptors_capacity,
        u32 textures_capacity, 
        u32 textures_width,
        u32 textures_height
    );

    void init();

    u32 addTexture(const void* data);
    u32 addMaterialParams(MaterialParams params);
    void addMaterial(MaterialDescriptor descriptor);

    void deinit();
};
 
}

#endif