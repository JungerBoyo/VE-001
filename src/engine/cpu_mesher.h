#ifndef VE001_CPU_MESHER_H
#define VE001_CPU_MESHER_H

#include <span>
#include <thread>
#include <atomic>
#include <future>
#include <bitset>

#include <vmath/vmath.h>

#include "threadsafe_ringbuffer.h"
#include "engine_context.h"

#include "vertex.h"
#include "meshing_engine_base.h"

#ifdef ENGINE_TEST
#include <timer.h>
#endif

namespace ve001 {

struct CpuMesher {
	struct Promise {
		std::span<const Vertex> staging_buffer_ptr;
		std::atomic<bool>* staging_buffer_in_use_flag{ nullptr };
		std::array<vmath::u32, 6> written_quads{{0}};
        bool overflow_flag{ false };
#ifdef ENGINE_TEST
		Timer cmd_timer_meshing;
		Timer cmd_timer_real;
#endif
	};
	struct MeshingTask {
		std::promise<Promise> promise;
		vmath::Vec3f32 chunk_position;
		std::span<const vmath::u16> voxel_data;
	};
	struct Thread {
		std::array<std::vector<Vertex>, 1> staging_buffers;
		std::array<std::atomic<bool>, 1> staging_buffer_in_use_flags;
		std::size_t current_buffer{ 0UL };
		std::jthread jthread;
		Thread() : staging_buffer_in_use_flags{}  {
			for (auto& f : staging_buffer_in_use_flags)
				f.store(false, std::memory_order_relaxed);
		}
		Thread(const Thread& other) {
			for (std::size_t i{ 0 }; i < staging_buffer_in_use_flags.size(); ++i)
				staging_buffer_in_use_flags[i].store(
						other.staging_buffer_in_use_flags[i].load(std::memory_order_relaxed),
						std::memory_order_relaxed);
		}
	};
	struct GreedyMeshingPromise {
		vmath::u32 written_quads{ 0 };
        bool overflow_flag{ false };
	};
	struct GreedyMeshingFaceDescriptor {
		Face face;
		vmath::u32 axis;
		vmath::Vec3u32 logical_indices;
		vmath::Vec3u32 real_indices;
		vmath::Vec3i32 logical_extent;
		vmath::i32 edge_value;
		vmath::i32 polarity;
		vmath::Vec2u32 squashed_extent_logical_indices;
		vmath::i32 plane_size;
		vmath::u64 max_submesh_size;
	};
    /// @brief allocated threads
	std::vector<Thread> _threads;
    /// @brief counts number of threads' which are already
    /// initialized
    std::atomic_int32_t _ready_counter{ 0 };
    /// @brief threads' exit condition
    std::atomic_bool _done{ false };
	/// @brief signals to thread that buffers are too small
	/// and it should stop meshing until updateLimits is called
	//std::atomic_bool overflowed{ false };
	bool overflowed{ false };
	std::condition_variable cond_overflowed;
	std::mutex m_overflowed;
	/// @brief promise queue for meshing task
	ThreadSafeRingBuffer<MeshingTask> _meshing_tasks;

	const EngineContext& _engine_context;

	CpuMesher(const EngineContext& engine_context, vmath::u32 threads_count, std::size_t capacity) noexcept;
	
	void updateLimits() noexcept;
    /// @brief threads' entry point
    void thread(std::size_t thread_index) noexcept;
    /// @brief signals if chunk data streamer was fully initialized
    /// @return true if chunk data streamer is initialzed
    bool ready() const noexcept { return _ready_counter == _threads.size(); }
	/// @brief meshes a chunk
	/// @param chunk_position real position of the chunk
	/// @param voxel_data voxel data/chunk data from which mesh should be built
	std::future<Promise> mesh(vmath::Vec3f32 chunk_position, std::span<const vmath::u16> voxel_data) noexcept;

	Promise greedyMeshing(std::vector<Vertex>& out, 
			vmath::Vec3f32 chunk_position,
			std::span<const vmath::u16> voxel_data) noexcept;
	GreedyMeshingPromise greedyMeshingFace(std::span<Vertex> out, 
			vmath::Vec3f32 chunk_position,
			std::span<const vmath::u16> voxel_data,
			GreedyMeshingFaceDescriptor
	) noexcept;

	~CpuMesher() noexcept { _done = true; }
};

}

#endif
