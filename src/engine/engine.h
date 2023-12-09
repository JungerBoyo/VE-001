#ifndef VE001_ENGINE_H
#define VE001_ENGINE_H

#include <vmath/vmath.h>

#include "engine_context.h"
#include "shader_repository.h"
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
        /// @brief growth coefficient of the pool. Value of overflow is multiplied
        /// by this coefficient deciding new max size of the chunk pool
        vmath::f32 chunk_pool_growth_coefficient;
        /// @brief meshing shader step/local group size. It should be the same
        /// as local_size_x in greedy meshing shader
        vmath::i32 meshing_shader_local_group_size;
    };

    /// @brief if true partitioning is used based on applied partitions
    bool partitioning{ false };
    /// @brief engine context containing common metadata for modules
    EngineContext _engine_context;
    /// @brief world grid holding chunk pool and managing 
    /// state of the visibility of chunks
    WorldGrid _world_grid;

    /// @brief constructor doesn't initialize any opengl resources
    /// @param config confiugration structure 
    Engine(Config config);
    /// @brief initalization of the opengl resources
    void init();

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
    );

    /// @brief passes the custom partitioning call to internal partition function
    /// @tparam ...Args types of aux arguments to pass to unary_op function
    /// @param unary_op unary operation which is criterion based on which the commands are partitioned
    /// @param use_last_partition If true then the previous parition will be paritioned again
    /// @param args Aux arguments to pass to unary_op function
    template<typename ...Args> 
    void applyCustomPartition(bool(*unary_op)(Face orientation, vmath::Vec3f32 position, Args... args), bool use_last_partition, Args&&... args) {
        _world_grid._chunk_pool.partitionDrawCommands(unary_op, use_last_partition, std::forward<Args>(args)...);
    }

    /// @brief updates world grid state based on camera position
    /// @param position new camera position
    void updateCameraPosition(vmath::Vec3f32 position);
    /// @brief polls for chunks updates (non blocking!)
    /// @return true if new chunk was loaded
    bool pollChunksUpdates();
    /// @brief updates draw state, binds vao, vbo. If draw command buffer is dirty
    /// supplies draw commands to gpu
    void updateDrawState();
    /// @brief draws a scene
    void draw();
    
    /// @brief deinitializes opengl resources
    void deinit();
};

}

#endif