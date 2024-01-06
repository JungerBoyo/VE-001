#include "gpu_buffer.h"

#include <glad/glad.h>

using namespace ve001;
using namespace vmath;

Error GPUBuffer::init() noexcept {
    glCreateBuffers(1, &_id);
    glNamedBufferStorage(_id, _size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    if (glGetError() == GL_OUT_OF_MEMORY) {
        return Error::GPU_ALLOCATION_FAILED;
    }
    return Error::NO_ERROR;
}
void GPUBuffer::write(const void* data) noexcept {
    glNamedBufferSubData(_id, 0, _size, data);
}
void GPUBuffer::write(const void* data, u32 offset, u32 size) noexcept {
    glNamedBufferSubData(_id, offset, size, data);
}
void GPUBuffer::read(void* data, u32 offset, u32 size) noexcept {
    glGetNamedBufferSubData(_id, offset, size, data);
}
void GPUBuffer::bind(u32 target, u32 binding) noexcept {
    glBindBufferBase(target, binding, _id);
}
void GPUBuffer::deinit() noexcept {
    glDeleteBuffers(1, &_id);
}