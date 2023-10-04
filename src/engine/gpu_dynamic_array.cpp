#include "gpu_dynamic_array.h"
#include <glad/glad.h>

#include <cstring>
#include <vector>
#include <numeric>

using namespace vmath;
using namespace ve001;

Error GPUDynamicArray::init(u32 capacity) noexcept {
    glCreateBuffers(1, &_ssbo_id);

    glNamedBufferStorage(
        _ssbo_id,
        capacity * _element_size,
        nullptr,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    _ssbo_ptr = glMapNamedBufferRange(
        _ssbo_id,
        0U,
        capacity * _element_size,
        GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT
    );

    if (_ssbo_ptr == nullptr) {
        deinit();
        return Error::FAILED_TO_MAP_SSBO_BUFFER;
    }

    _capacity = capacity;

    return Error::NO_ERROR;
}

void GPUDynamicArray::deinit() noexcept {
    if (_ssbo_ptr != nullptr) {
        glUnmapNamedBuffer(_ssbo_id);
    }
    glDeleteBuffers(1, &_ssbo_id);
}

Error GPUDynamicArray::pushBack(const void* data) noexcept {
    if (_size == _capacity) {
        const auto result = reserve(static_cast<u32>(GROW_FACTOR * static_cast<f32>(_capacity)));
        if (result != Error::NO_ERROR) {
            return result;
        }
    }

    std::memcpy(static_cast<u8*>(_ssbo_ptr) + (_size * _element_size), data, _element_size);
    ++_size;
    
    return Error::NO_ERROR;
}

void GPUDynamicArray::popBack() noexcept {
    if (_size > 0U) {
        --_size;
    }
}

Error GPUDynamicArray::reserve(u32 capacity) noexcept {
    if (capacity > _capacity) {
        deinit();

        std::vector<u8> data(_element_size * _size, 0U);
        std::memcpy(static_cast<void*>(data.data()), _ssbo_ptr, data.size());
        
        const auto result = init(capacity);
        if (result != Error::NO_ERROR) {
            return result;
        }

        std::memcpy(_ssbo_ptr, static_cast<const void*>(data.data()), data.size());
    }

    return Error::NO_ERROR;
}

Error GPUDynamicArray::resize(u32 size) noexcept {
    if (size > _capacity) {
        const auto result = reserve(size);
        if (result != Error::NO_ERROR) {
            return result;
        }
    }
    _size = size;

    return Error::NO_ERROR;
}

Error GPUDynamicArray::write(const void* data, u32 count, u32 offset) noexcept {
    if (offset + count > _size) {
        return Error::GPU_DYNAMIC_ARRAY_WRITE_OVERFLOW;
    }
    std::memcpy(static_cast<u8*>(_ssbo_ptr) + offset * _element_size, data, count * _element_size);

    return Error::NO_ERROR;
}

void GPUDynamicArray::bind(u32 binding_index) noexcept {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_index, _ssbo_id);
}