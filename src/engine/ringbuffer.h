#ifndef VE001_RING_BUFFER_H
#define VE001_RING_BUFFER_H

#include <vector>

namespace ve001 {

template<typename T>
struct RingBuffer {
    std::vector<T> _buffer;
    std::size_t _writer_index{ 0U };
    std::size_t _reader_index{ 0U };

    RingBuffer() = default;

    RingBuffer(std::size_t size, T fill_value) 
        : _buffer(size, fill_value) {}

    bool pushBack(T value) {
        const auto next_writer_index = (_writer_index + 1) % _buffer.size();
        if (next_writer_index == _reader_index) {
            return false;
        }
        _buffer[_writer_index] = value;
        _writer_index = next_writer_index;
        return true;
    }

    bool popBack(T& value) {
        const auto next_reader_index = (_reader_index + 1) % _buffer.size();
        if (next_reader_index == _writer_index) {
            return false;
        }
        value = _buffer[_reader_index];
        _reader_index = next_reader_index;
        return true;
    }

    T back() const {
        return _buffer[_reader_index];
    }

    bool empty() const {
        return _writer_index == _reader_index;
    }

    void clear() {
        _buffer.clear();
        _writer_index = 0U;
        _reader_index = 0U;
    }
};

}
#endif