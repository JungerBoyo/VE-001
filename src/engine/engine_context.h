#ifndef VE001_ENGINE_CONTEXT_H
#define VE001_ENGINE_CONTEXT_H

#include <vmath/vmath.h>

#include "shader_repository.h"

namespace ve001 {

/// @brief context holds common state for different engine components/modules
struct EngineContext {
    /// @brief repository with all shaders
    ShaderRepository shader_repo;
    /// @brief chunk size/resolution 
    vmath::Vec3i32 chunk_size;
    /// @brief half chunk size/resolution 
    vmath::Vec3i32 half_chunk_size;
    /// @brief chunk size/resolution in 1D
    vmath::u64 chunk_size_1D;
    /// @brief chunk size in voxel context (single voxel chunk)
    vmath::u64 chunk_voxel_data_size;
    /// @brief maximum possible mesh size of a single chunk's side's indices after greedy meshing
    vmath::u64 chunk_max_possible_submesh_indices_size;
    /// @brief maximum possible mesh size of a single chunk after greedy meshing
    vmath::u64 chunk_max_possible_mesh_size;
    /// @brief maximum possible mesh size of a single chunk's side after greedy meshing
    vmath::u64 chunk_max_possible_submesh_size;
    /// @brief maximum current mesh size of a single chunk's side's indices after greedy meshing
    // vmath::u64 chunk_max_current_submesh_indices_size;
    /// @brief maximum current mesh size of a single chunk after greedy meshing
    vmath::u64 chunk_max_current_mesh_size;
    /// @brief maximum current mesh size of a single chunk's side after greedy meshing
    vmath::u64 chunk_max_current_submesh_size;
    /// @brief it maps to local_size_x attribute in greedy meshing compute shader
    vmath::i32 meshing_axis_progress_step;
};

}

#endif