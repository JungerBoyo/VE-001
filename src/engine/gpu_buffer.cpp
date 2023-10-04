#include "gpu_buffer.h"

#include <glad/glad.h>

using namespace ve001;
using namespace vmath;

void GPUBuffer::init() {
    glCreateBuffers(1, &_id);
    glNamedBufferStorage(_id, _size, nullptr, GL_DYNAMIC_STORAGE_BIT);
}
void GPUBuffer::init(u32 id) {
    _id = id;
    glNamedBufferStorage(_id, _size, nullptr, GL_DYNAMIC_STORAGE_BIT);
}
void GPUBuffer::write(const void* data) {
    glNamedBufferSubData(_id, 0, _size, data);
}
void GPUBuffer::write(const void* data, u32 offset, u32 size) {
    glNamedBufferSubData(_id, offset, size, data);
}
void GPUBuffer::bind(u32 target, u32 binding) {
    glBindBufferBase(target, binding, _id);
}
void GPUBuffer::deinit() {
    glDeleteBuffers(1, &_id);
}