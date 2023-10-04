#ifndef VE001_GPU_OBJECT_POOLING_RESOURCE_H
#define VE001_GPU_OBJECT_POOLING_RESOURCE_H

#include "gpu_dynamic_array.h"

#include <concepts>
#include <vector>

namespace ve001 {

template<typename T>
concept GPUObject = requires(T& object) {
    { object.data() } -> std::same_as<const void*>;
    { T::size() } -> std::convertible_to<std::size_t>;
};

template<typename T> 
requires GPUObject<T>
struct GPUObjectPoolingResource {
    static constexpr vmath::u32 INVALID_ID{ std::numeric_limits<vmath::u32>::max() };

    struct Object {
        vmath::u32 id;
        T value;
    };

    std::vector<Object> _objects;
    GPUDynamicArray _buffer{T::size()}; 
    std::vector<vmath::u32> _id_to_index;
    std::vector<vmath::u32> _free_ids;

    void init(u32 capacity) {
        _objects.reserve(capacity);
        _buffer.init(capacity);
        _id_to_index.reserve(capacity);
    }

    vmath::u32 add(const T& object) {
        if (!_free_ids.empty()) {
            const auto id = _free_ids.back();
            _free_ids.pop_back();
            _id_to_index[id] = _objects.size();
            _objects.emplace_back(id, object);
            _buffer.pushBack(object.data());
            return id;
        }
        _id_to_index.push_back(_objects.size());
        _objects.emplace_back(_id_to_index.back(), object);
        _buffer.pushBack(object.data());
        return _id_to_index.back();
    }
    T& get(vmath::u32 id) {
        const auto index = _id_to_index[id];
        return _objects[index].value;
    }
    void update(vmath::u32 id) {
        const auto index = _id_to_index[id];
        const auto& object = _objects[index];
        _buffer.write(object.value.data(), 1, index);
    }
    void del(vmath::u32 id) {
        const auto index = _id_to_index[id];
        if (index == _objects.size() - 1) {
            _objects.pop_back();
            _buffer.popBack();
        } else {
            const auto& last_object = _objects.back();
            _objects[index] = last_object;
            _buffer.write(last_object.data(), 1, index);
            _id_to_index[last_object.id] = index;
            _objects.pop_back();
            _buffer.popBack();
        }
        if (id == _id_to_index.size() - 1) {
            _id_to_index.pop_back();
        } else {
            _id_to_index[id] = INVALID_ID;
            _free_ids.push_back(id);
        }
    }

    void deinit() {
        _buffer.deinit();
    }
};

}

#endif
