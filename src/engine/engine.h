#ifndef VE001_ENGINE_H
#define VE001_ENGINE_H

#include <filesystem>
#include <optional>

#include <vmath/vmath.h>

#include "engine_context.h"
#include "world_grid.h"
#include "vertex.h"

namespace ve001 {

/// @brief main abstraction class to voxel rendering engine
struct Engine {
    /// @brief configuration structuer
    struct Config {
        /// @brief world size as semi axes of an ellipsoid
        vmath::Vec3f32 world_size;
        /// @brief initial camera position
        vmath::Vec3f32 initial_position;
        /// @brief resolution in voxels of the single chunk
        vmath::Vec3i32 chunk_size;
        /// @brief data generator called to generate data
        std::unique_ptr<ChunkGenerator> chunk_data_generator;
        /// @brief configures number of threads used by ChunkDataStreamer
        /// if 0 then the number of threads will that of the number of CPU threads
        vmath::u32 chunk_data_streamer_threads_count;
        /// @brief growth coefficient of the pool. Value of overflow is multiplied
        /// by this coefficient deciding new max size of the chunk pool
        vmath::f32 chunk_pool_growth_coefficient;
        /// @brief meshing shader step/local group size. It should be the same
        /// as local_size_x in greedy meshing shader
        vmath::i32 meshing_shader_local_group_size{ 64 };
		/// @brief if to use GPU based meshing engine
		bool use_gpu_meshing_engine;
		/// @brief if to use GPU based meshing engine
		vmath::i32 cpu_mesher_threads_count;
#ifdef USE_VOLUME_TEXTURE_3D
		/// @brief path to meshing shader src
		std::filesystem::path meshing_shader_src_path{"./shaders/src/greedy_meshing_shader/optshader.comp"};
		/// @brief path to meshing shader bin in spirv (optional)        
		std::optional<std::filesystem::path> meshing_shader_bin_path{"./shaders/bin/greedy_meshing_shader/optcomp.spv"};
#else
		/// @brief path to meshing shader src
		std::filesystem::path meshing_shader_src_path{"./shaders/src/greedy_meshing_shader/shader.comp"};
		/// @brief path to meshing shader bin in spirv (optional)        
		std::optional<std::filesystem::path> meshing_shader_bin_path{"./shaders/bin/greedy_meshing_shader/comp.spv"};
#endif
    };

    /// @brief engine's error flags 
    Error error{ Error::NO_ERROR };

    /// @brief if true partitioning is used based on applied partitions
    bool partitioning{ false };
    /// @brief engine context containing common metadata for modules
    EngineContext _engine_context;
    /// @brief world grid holding chunk pool and managing 
    /// state of the visibility of chunks
    WorldGrid _world_grid;

    /// @brief constructor doesn't initialize any opengl resources
    /// @param config confiugration structure 
    Engine(Config config) noexcept;

    /// @brief applies frustum culling based on separating axis theorem
    /// @param use_last_partition wether to use last partitioning
    /// @param z_near near plane
    /// @param z_far far plane
    /// @param x_near half width of the near plane of the frustum = eg.
    /// aspect_ratio * tan(view_angle/2) * z_near
    /// @param y_near half width of the near plane of the frustum = eg.
    /// tan(view_angle/2) * z_near
    /// @param view_matrix view matrix of the used camera
    void applyFrustumCullingPartition(
        bool use_last_partition, 
        vmath::f32 z_near, 
        vmath::f32 z_far, 
        vmath::f32 x_near,
        vmath::f32 y_near,
        vmath::Mat4f32 view_matrix
    ) noexcept;

    /// @brief passes the custom partitioning call to internal partition function
    /// @tparam ...Args types of aux arguments to pass to unary_op function
    /// @param unary_op unary operation which is criterion based on which the commands are partitioned
    /// @param use_last_partition If true then the previous parition will be paritioned again
    /// @param args Aux arguments to pass to unary_op function
    template<typename ...Args> 
    void applyCustomPartition(bool(*unary_op)(Face orientation, vmath::Vec3f32 position, Args... args), bool use_last_partition, Args... args) noexcept {
        _world_grid._chunk_pool.partitionDrawCommands(unary_op, use_last_partition, args...);
    }

    /// @brief initalization of the opengl resources
    /// @return true if there were errors during engine's initialization
    bool init() noexcept;
    /// @brief updates world grid state based on camera position
    /// @param position new camera position
    void updateCameraPosition(vmath::Vec3f32 position) noexcept;
    /// @brief polls for chunks updates (non blocking!)
    /// @return true if new chunk was loaded
    bool pollChunksUpdates() noexcept;
    /// @brief updates draw state, binds vao, vbo. If draw command buffer is dirty
    /// supplies draw commands to gpu
    void updateDrawState() noexcept;
    /// @brief draws a scene
    void draw() noexcept;
    /// @brief deinitializes opengl resources
    void deinit() noexcept;
};

}

#endif
