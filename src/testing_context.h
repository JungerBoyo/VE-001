#ifndef TESTING_CONTEXT_H
#define TESTING_CONTEXT_H

#include "timer.h"
#include <vmath/vmath.h>
#include <glad/glad.h>

struct TestingContext {
    vmath::u32 prims_generated_query{ 0U };
    vmath::u32 samples_passed_query{ 0U };
    vmath::u32 gpu_frame_time_elapsed_query{ 0U };

    vmath::u64 frame_counter{ 0UL };
    vmath::u64 mean_prims_generated{ 0U };
    vmath::u64 mean_samples_passed{ 0U };
    vmath::u64 gpu_mean_frame_time_elapsed_ns{ 0U };
    vmath::u64 gpu_max_frame_time_elapsed_ns{ 0U };
    vmath::u64 cpu_mean_frame_time_elapsed_ns{ 0U };
    vmath::u64 cpu_max_frame_time_elapsed_ns{ 0U };

    Timer cpu_timer;

    void init() {
        glCreateQueries(GL_PRIMITIVES_GENERATED, 1, &prims_generated_query);
        glCreateQueries(GL_SAMPLES_PASSED, 1, &samples_passed_query);
        glCreateQueries(GL_TIME_ELAPSED, 1, &gpu_frame_time_elapsed_query);
    }

    void beginMeasure() {
        glBeginQuery(GL_TIME_ELAPSED, gpu_frame_time_elapsed_query);
        cpu_timer.start();
        glBeginQuery(GL_PRIMITIVES_GENERATED, prims_generated_query);
        glBeginQuery(GL_SAMPLES_PASSED, samples_passed_query);

        ++frame_counter;
    }
    void endMeasure() {
        cpu_timer.stop();
        glEndQuery(GL_PRIMITIVES_GENERATED);
        glEndQuery(GL_SAMPLES_PASSED);
        glEndQuery(GL_TIME_ELAPSED);

        vmath::u64 prims_generated{ 0U };
        vmath::u64 samples_passed{ 0U };
        vmath::u64 gpu_frame_time_elapsed_ns{ 0U };
        glGetQueryObjectui64v(gpu_frame_time_elapsed_query, GL_QUERY_RESULT, &gpu_frame_time_elapsed_ns);
        glGetQueryObjectui64v(samples_passed_query, GL_QUERY_RESULT, &samples_passed);
        glGetQueryObjectui64v(prims_generated_query, GL_QUERY_RESULT, &prims_generated);

        if (mean_prims_generated == 0U) {
            mean_prims_generated = prims_generated;
        } else {
            mean_prims_generated = (mean_prims_generated + prims_generated)/2UL;
        }
        if (gpu_mean_frame_time_elapsed_ns == 0U) {
            gpu_mean_frame_time_elapsed_ns = gpu_frame_time_elapsed_ns;
        } else {
            gpu_mean_frame_time_elapsed_ns = (gpu_mean_frame_time_elapsed_ns + gpu_frame_time_elapsed_ns)/2UL;
        }
        if (mean_samples_passed == 0U) {
            mean_samples_passed = samples_passed;
        } else {
            mean_samples_passed = (mean_samples_passed + samples_passed)/2UL;
        }
        if (cpu_mean_frame_time_elapsed_ns == 0U) {
            cpu_mean_frame_time_elapsed_ns = cpu_timer.duration;
        } else {
            cpu_mean_frame_time_elapsed_ns = (cpu_mean_frame_time_elapsed_ns + cpu_timer.duration)/2UL;
        }
        if (gpu_max_frame_time_elapsed_ns < gpu_frame_time_elapsed_ns) {
            gpu_max_frame_time_elapsed_ns = gpu_frame_time_elapsed_ns;
        } 
        if (cpu_max_frame_time_elapsed_ns < cpu_timer.duration) {
            cpu_max_frame_time_elapsed_ns = cpu_timer.duration;
        } 
    }

    void deinit() {
        vmath::u32 tmp[3] = { prims_generated_query, samples_passed_query, gpu_frame_time_elapsed_query};
        glDeleteQueries(3, tmp);
    }
};

#endif