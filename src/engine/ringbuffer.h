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

    RingBuffer(std::size_t size)
        : _buffer(size) {}
    RingBuffer(std::size_t size, T fill_value)
        : _buffer(size, fill_value) {}

    void resize(std::size_t new_size) {
        _buffer.resize(new_size);
    }

    bool write(T value) noexcept {
        if (_writer_index == _reader_index && !_empty) {
            return false;
        }
        _empty = false;
        _buffer[_writer_index] = std::move(value);
        _writer_index = (_writer_index + 1) % _buffer.size();

        return true;
    }

    bool read(T& value) noexcept {
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
    void emptyRead() noexcept {
        if (_empty) {
            return;
        }
        _reader_index = (_reader_index + 1) % _buffer.size();

        if (_reader_index == _writer_index) {
            _empty = true;
        }
    }

    bool peek(T& value) const noexcept {
        if (_empty) {
            return false;
        }
        value = _buffer[_reader_index];
        return true;
    }
    bool peek(T*& value) noexcept {
        if (_empty) {
            return false;
        }
        value = &_buffer[_reader_index];
        return true;
    }

    bool empty() const noexcept {
        return _empty;
    }

    void clear() {
        _writer_index = 0U;
        _reader_index = 0U;
        _empty = true;
    }
};

}
#endif