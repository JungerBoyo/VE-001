#ifndef TIMER_H
#define TIMER_H

#include <chrono>

struct Timer {
    std::uint64_t duration;
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_timer;

    void start() {
        duration = 0UL;
        begin_timer = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        const auto end_timer{std::chrono::high_resolution_clock::now()};
        
        const auto begin_ns{std::chrono::time_point_cast<std::chrono::nanoseconds>(begin_timer).time_since_epoch().count()};
        const auto end_ns{std::chrono::time_point_cast<std::chrono::nanoseconds>(end_timer).time_since_epoch().count()};

        duration = static_cast<std::uint64_t>(end_ns - begin_ns);
    }
};

#endif