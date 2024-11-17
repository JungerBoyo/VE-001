#ifndef VE001_MESHING_ENGINE_BASE_H
#define VE001_MESHING_ENGINE_BASE_H

#include <span>

#include "chunk_id.h"
#include "engine_context.h"
#include "ringbuffer.h"

#ifdef ENGINE_TEST
#include <tuple>
#endif

namespace ve001 {

struct MeshingEngineBase {
    /// @brief command descripting meshing execution of a single chunk
    struct Command {
        /// @brief id of chunk meshed by command
        ChunkId chunk_id; 
        /// @brief position of the chunk
        vmath::Vec3f32 chunk_position;
        /// @brief pointer to voxel data to be issued before meshing starts
        std::span<const vmath::u16> voxel_data;
        /// @brief gl fence for which to wait in case the command is active one
        /// also indicates !!!IF COMMAND IS INITIALIZED (nullptr here if not)!!!
        void* fence{ nullptr };
        /// @brief axis progress keeps track of how many planes on each axes were meshed
        vmath::i32 axis_progress;
    };

    /// @brief is an interface and holds completed command data 
    struct Result {
        /// @brief indentifier of a succesfully meshed chunk (maps to
        /// chunk id in ChunkPool)
        ChunkId chunk_id;
        /// @brief written indices count *per face* indexed with ve001::Face enum.
        /// determines how many indices to render
        std::array<vmath::u32, 6> written_indices;
        /// @brief if true then number of potentially written vertices is
        /// bigger than current chunk region size and pool needs to be extended
        bool overflow_flag{ false };
    };

    const EngineContext& _engine_context;

    MeshingEngineBase(const EngineContext& engine_context) noexcept;

    virtual void init(vmath::u32 vbo_id) noexcept = 0;

    /// @brief issues meshing command to the engine
    /// @param chunk_id id of processed chunk. Maps to chunk ids in ChunkPool
    /// @param chunk_position chunk position
    /// @param voxel_data pointer to voxel_data based on which the meshing will take place
    virtual void issueMeshingCommand(ChunkId chunk_id, vmath::Vec3f32 chunk_position, std::span<const vmath::u16> voxel_data) noexcept = 0;

    /// @brief Function polls for the result from next command. It isn't waiting (the call
    // is non blocking), only checks once.
    /// @param result variable to which the function write result into
    /// @return true if valid value was written into the <future> param false if not
    virtual bool pollMeshingCommand(Result& result) noexcept = 0;

    /// @brief updates metadata based on engine context and new vbo id
    /// @param new_vbo_id new vbo id to which to write meshes
    virtual void updateMetadata(vmath::u32 new_vbo_id) noexcept = 0;

#ifdef ENGINE_TEST
	virtual std::tuple<vmath::u64, vmath::u64, vmath::u64>
		getBenchmarkData() const noexcept = 0;
#endif

    virtual void deinit() noexcept {}
};

}
#endif
