#ifndef VE001_THREAD_POOL_H
#define VE001_THREAD_POOL_H

#include <threadsafe_ringbuffer.h>

#include <atomic>
#include <functional>
#include <vector>
#include <thread>
#include <future>

namespace ve001 {

struct ThreadPool {
    std::atomic_bool _done;
    std::vector<std::jthread> _threads;
    ThreadSafeRingBuffer<std::move_only_function<void()>> _tasks;

    ThreadPool() : _done(false) {
        try {
            const auto threads_count = std::thread::hardware_concurrency();
            for (std::uint32_t i{ 0U }; i < threads_count; ++i) {
                _threads.push_back(std::jthread(&ThreadPool::thread, this));
            }
        } catch([[maybe_unused]] const std::exception&) {
            _done = true;
        }
    }

    void thread() {
        while (!_done) {
            std::move_only_function<void()> task;
            if (_tasks.waitingRead(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
        }
    }

    template<typename Fn>
    std::future<typename std::result_of<Fn()>::type> submit(Fn f) {
        using R = typename std::result_of<Fn()>::type;

        std::packaged_task<R()> packaged_task(std::move(f));
        std::future<R> result(task.get_future());

        _tasks.write([task = std::move(packaged_task)] mutable { task(); });

        return result;
    }

    ~ThreadPool() { _done = true; }
};

}

#endif