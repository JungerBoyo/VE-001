#ifndef TESTING_CONTEXT_H
#define TESTING_CONTEXT_H

#include <vector>
#include <fstream>

#include <vmath/vmath.h>
#include <glad/glad.h>

#include "timer.h"

struct TestingContext {
    struct SampleFrame {
        vmath::u64 prims_generated{ 0U };
        vmath::u64 samples_passed{ 0U };
        vmath::u64 gpu_frame_time_elapsed_ns{ 0U };
        vmath::u64 cpu_frame_time_elapsed_ns{ 0U };
        vmath::u64 chunks_in_use{ 0U };
        vmath::u64 gpu_active_memory_usage_in_use{ 0U };
        vmath::u64 gpu_active_memory_usage_real{ 0U };
        vmath::u64 gpu_passive_memory_usage{ 0U };
        vmath::u64 cpu_active_memory_usage{ 0U };
        vmath::u64 cpu_passive_memory_usage{ 0U };
    };

    struct SampleMeshing {
        vmath::u64 gpu_meshing_time_elapsed_ns{ 0U };
        vmath::u64 real_meshing_time_elapsed_ns{ 0U };
        vmath::u64 gpu_meshing_setup_time_elapsed_ns{ 0U };
    };

    vmath::u32 _prims_generated_query{ 0U };
    vmath::u32 _samples_passed_query{ 0U };
    vmath::u32 _gpu_frame_time_elapsed_query{ 0U };

    Timer _cpu_timer;

    vmath::u64 _frame_counter{ 0UL };
    vmath::u32 _max_frames;

    std::vector<SampleFrame> _frame_samples;
    std::vector<SampleMeshing> _meshing_samples;

    TestingContext(vmath::u32 max_frames) 
        : _max_frames(max_frames) {
        _frame_samples.reserve(max_frames); 
        _meshing_samples.reserve(max_frames);
    }

    void init() {
        glCreateQueries(GL_PRIMITIVES_GENERATED, 1, &_prims_generated_query);
        glCreateQueries(GL_SAMPLES_PASSED, 1, &_samples_passed_query);
        glCreateQueries(GL_TIME_ELAPSED, 1, &_gpu_frame_time_elapsed_query);
    }

    void saveMeshingSample(SampleMeshing sample) {
        _meshing_samples.push_back(sample);
    }

    void beginMeasure() {
        if (_frame_counter < _max_frames) {
            glBeginQuery(GL_TIME_ELAPSED, _gpu_frame_time_elapsed_query);
            _cpu_timer.start();
            glBeginQuery(GL_PRIMITIVES_GENERATED, _prims_generated_query);
            glBeginQuery(GL_SAMPLES_PASSED, _samples_passed_query);
        }
    }
    void endMeasure(
        vmath::u64 chunks_in_use,
        vmath::u64 gpu_active_memory_usage_in_use,
        vmath::u64 gpu_active_memory_usage_real,
        vmath::u64 gpu_passive_memory_usage,
        vmath::u64 cpu_active_memory_usage,
        vmath::u64 cpu_passive_memory_usage) {

        if (_frame_counter < _max_frames) {
            _cpu_timer.stop();
            glEndQuery(GL_PRIMITIVES_GENERATED);
            glEndQuery(GL_SAMPLES_PASSED);
            glEndQuery(GL_TIME_ELAPSED);

            vmath::u64 prims_generated{ 0U };
            vmath::u64 samples_passed{ 0U };
            vmath::u64 gpu_frame_time_elapsed_ns{ 0U };
            glGetQueryObjectui64v(_gpu_frame_time_elapsed_query, GL_QUERY_RESULT, &gpu_frame_time_elapsed_ns);
            glGetQueryObjectui64v(_samples_passed_query, GL_QUERY_RESULT, &samples_passed);
            glGetQueryObjectui64v(_prims_generated_query, GL_QUERY_RESULT, &prims_generated);

            _frame_samples.emplace_back(
                prims_generated,
                samples_passed,
                gpu_frame_time_elapsed_ns,
                _cpu_timer.duration,
                chunks_in_use,
                gpu_active_memory_usage_in_use,
                gpu_active_memory_usage_real,
                gpu_passive_memory_usage,
                cpu_active_memory_usage,
                cpu_passive_memory_usage
            );
        }

        ++_frame_counter;
    }
    
    void dumpFrameSamples() {
        std::ofstream stream("./ve001_frame_samples.csv");

        const std::string header = 
            std::string("prims_generated") + 
            std::string(",samples_passed") +
            std::string(",gpu_frame_time_elapsed_ns") +
            std::string(",cpu_frame_time_elapsed_ns") +
            std::string(",chunks_in_use") +
            std::string(",gpu_active_memory_usage_in_use") + 
            std::string(",gpu_active_memory_usage_real") +
            std::string(",gpu_passive_memory_usage") +
            std::string(",cpu_active_memory_usage") +
            std::string(",cpu_passive_memory_usage\n");

        stream.write(header.data(), header.size());

        for (const auto& sample : _frame_samples) {
            const std::string line = 
                std::to_string(sample.prims_generated) + "," +
                std::to_string(sample.samples_passed) + "," +
                std::to_string(sample.gpu_frame_time_elapsed_ns) + "," +
                std::to_string(sample.cpu_frame_time_elapsed_ns) + "," +
                std::to_string(sample.chunks_in_use) + "," +
                std::to_string(sample.gpu_active_memory_usage_in_use) + "," +
                std::to_string(sample.gpu_active_memory_usage_real) + "," +
                std::to_string(sample.gpu_passive_memory_usage) + "," +
                std::to_string(sample.cpu_active_memory_usage) + "," +
                std::to_string(sample.cpu_passive_memory_usage) + '\n';

            stream.write(line.data(), line.size());
        }
    }

    void dumpMeshingSamples() {
        std::ofstream stream("./ve001_meshing_samples.csv");

        const std::string header = 
            std::string("gpu_meshing_time_elapsed_ns,") +
            std::string("real_meshing_time_elapsed_ns,") +
			std::string("meshing_setup_time_elapsed_ns\n");

        stream.write(header.data(), header.size());

        for (const auto& sample : _meshing_samples) {
            const std::string line = 
                std::to_string(sample.gpu_meshing_time_elapsed_ns) + ',' +
                std::to_string(sample.real_meshing_time_elapsed_ns) + ',' +
                std::to_string(sample.gpu_meshing_setup_time_elapsed_ns) + '\n';
            stream.write(line.data(), line.size());
        }
    }

    void deinit() {
        vmath::u32 tmp[3] = { _prims_generated_query, _samples_passed_query, _gpu_frame_time_elapsed_query};
        glDeleteQueries(3, tmp);
    }
};

#endif
