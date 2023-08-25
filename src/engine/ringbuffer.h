#ifndef VE001_RING_BUFFER_H
#define VE001_RING_BUFFER_H

#include <vector>

namespace ve001 {

template<typename T>
struct RingBuffer {
    std::vector<T> _buffer;
    std::size_t _writer_index{ 0U };
    std::size_t _reader_index{ 0U };
    bool _empty{ true };

    RingBuffer() = default;

    RingBuffer(std::size_t size, T fill_value) 
        : _buffer(size, fill_value) {}

    bool pushBack(T value) {
        if (_writer_index == _reader_index && !_empty) {
            return false;
        }
        _empty = false;
        _buffer[_writer_index] = value;
        _writer_index = (_writer_index + 1) % _buffer.size();

        return true;
    }

    bool popBack(T& value) {
        if (_empty) {
            return false;
        }
        value = _buffer[_reader_index];
        _reader_index = (_reader_index + 1) % _buffer.size();

        if (_reader_index == _writer_index) {
            _empty = true;
        }
        return true;
    }

    T back() const {
        return _buffer[_reader_index];
    }

    bool empty() const {
        return _empty;
    }

    void clear() {
        _buffer.clear();
        _writer_index = 0U;
        _reader_index = 0U;
        _empty = true;
    }
};

}
#endif