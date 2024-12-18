#ifndef VE001_ENGINE_CONTEXT_H
#define VE001_ENGINE_CONTEXT_H

#include <optional>
#include <filesystem>

#include <vmath/vmath.h>

#include "enums.h"

namespace ve001 {

/// @brief context holds common state for different engine components/modules
struct EngineContext {
    /// @brief errors
    Error& error;
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
    /// @brief maximum current mesh size of a single chunk after greedy meshing
    vmath::u64 chunk_max_current_mesh_size;
    /// @brief maximum current mesh size of a single chunk's side after greedy meshing
    vmath::u64 chunk_max_current_submesh_size;
    /// @brief chunk pool memory growth coefficient
    vmath::f32 chunk_pool_growth_coefficient;
    /// @brief it maps to local_size_x attribute in greedy meshing compute shader
    vmath::i32 meshing_axis_progress_step{ 64 };
	/// @brief if to use GPU based meshing engine
	bool use_gpu_meshing_engine;
	/// @brief if to use GPU based meshing engine
	vmath::i32 cpu_mesher_threads_count;
    /// @brief path to meshing shader src
    std::filesystem::path meshing_shader_src_path;
    /// @brief path to meshing shader bin in spirv (optional)        
    std::optional<std::filesystem::path> meshing_shader_bin_path;
};

}

#endif
