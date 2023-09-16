#include "material_allocator.h"

using namespace vmath;
using namespace ve001;

MaterialAllocator::MaterialAllocator(
    u32 material_params_capacity, 
    u32 material_descriptors_capacity,
    u32 textures_capacity, 
    u32 textures_width,
    u32 textures_height
) : _texture_rgba8_array(textures_width, textures_height, textures_capacity),
    _material_params_array(sizeof(MaterialParams)),
    _material_descriptors_array(sizeof(MaterialDescriptor))
{}

void MaterialAllocator::init() {
    _texture_rgba8_array.init();
    _material_descriptors_array.init(8);
    _material_params_array.init(8);
}

u32 MaterialAllocator::addTexture(const void* data) {
    return _texture_rgba8_array.pushBack(data);
}

u32 MaterialAllocator::addMaterialParams(MaterialParams params) {
    u32 id{ 0U };
    _material_params_array.pushBack(id, static_cast<const void*>(&params));
    return id;
}

void MaterialAllocator::addMaterial(MaterialDescriptor descriptor) {
    u32 id{ 0U };
    _material_descriptors_array.pushBack(id, static_cast<const void*>(&descriptor));
}

void MaterialAllocator::deinit() {
    _texture_rgba8_array.deinit();
    _material_descriptors_array.deinit();
    _material_params_array.deinit();
}