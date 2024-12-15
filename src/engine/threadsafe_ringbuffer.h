#ifndef VE001_THREAD_SAFE_RING_BUFFER_H
#define VE001_THREAD_SAFE_RING_BUFFER_H

#include <vector>
#include <mutex>
#include <condition_variable>

namespace ve001 {

template<typename T>
struct ThreadSafeRingBuffer {
    mutable std::mutex _mutex;
    std::condition_variable _cond_var;
    std::vector<T> _buffer;
    std::size_t _writer_index{ 0U };
    std::size_t _reader_index{ 0U };
    bool _empty{ true };

    ThreadSafeRingBuffer() noexcept = default;

    ThreadSafeRingBuffer(std::size_t size)
        : _buffer(size) {}

    void resize(std::size_t new_size) {
        _buffer.resize(new_size);
    }

    bool write(const T& value) noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (_writer_index == _reader_index && !_empty) {
            return false;
        }
        _empty = false;
        _buffer[_writer_index] = value;
        _writer_index = (_writer_index + 1) % _buffer.size();
        
        _cond_var.notify_one();

        return true;
    }

	template<typename ...Args>
    bool write(Args&& ...args) noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (_writer_index == _reader_index && !_empty) {
            return false;
        }
        _empty = false;
        _buffer[_writer_index] = {std::forward<Args>(args)...};
        _writer_index = (_writer_index + 1) % _buffer.size();
        
        _cond_var.notify_one();

        return true;
    }
	bool write(T&& value) && noexcept { 
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (_writer_index == _reader_index && !_empty) {
            return false;
        }
        _empty = false;
        _buffer[_writer_index] = std::move(value);
        _writer_index = (_writer_index + 1) % _buffer.size();
        
        _cond_var.notify_one();

        return true;
	}

    bool read(T& value) noexcept {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_empty) {
            return false;
        }
        value = std::move(_buffer[_reader_index]);
        _reader_index = (_reader_index + 1) % _buffer.size();

        if (_reader_index == _writer_index) {
            _empty = true;
        }
        return true;
    }

    void poll(T& value) noexcept {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond_var.wait(lock, [this] { return !this->_empty; });

        value = std::move(_buffer[_reader_index]);
        _reader_index = (_reader_index + 1) % _buffer.size();

        if (_reader_index == _writer_index) {
            _empty = true;
        }
    }

    bool empty() const noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        return _empty;
    }

    void clear() noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        _writer_index = 0U;
        _reader_index = 0U;
        _empty = true;
    }
};

}

#endif
