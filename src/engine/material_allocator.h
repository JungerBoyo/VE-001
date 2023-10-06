#ifndef VE001_MATERIAL_ALLOCATOR_H
#define VE001_MATERIAL_ALLOCATOR_H

#include "vmath/vmath.h"
#include "texture_array.h"
#include "gpu_dynamic_array.h"

namespace ve001 {

struct MaterialParams {
    vmath::Vec4f32 specular;
    vmath::Vec3f32 diffuse;
    vmath::f32 alpha;
};

struct MaterialDescriptor {
    vmath::u32 material_texture_index[6];
    vmath::u32 material_params_index[6];
};

// TODO: handle the case material deleation? 
// TODO2: multiple texture arrays?
struct MaterialAllocator {
    TextureArray _texture_rgba8_array;
    GPUDynamicArray _material_params_array;
    GPUDynamicArray _material_descriptors_array;

    MaterialAllocator(
        vmath::u32 material_params_capacity, 
        vmath::u32 material_descriptors_capacity,
        vmath::u32 textures_capacity, 
        vmath::u32 textures_width,
        vmath::u32 textures_height
    );

    void init();

    void addTexture(const void* data);
    void addMaterialParams(MaterialParams params);
    void addMaterial(MaterialDescriptor descriptor);

    void deinit();
};
 
}

#endif