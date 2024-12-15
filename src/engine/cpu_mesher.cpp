#include "cpu_mesher.h"

#include <cstring>

#include <iostream>

using namespace ve001;
using namespace vmath;

CpuMesher::CpuMesher(const EngineContext& engine_context, vmath::u32 threads_count,
		std::size_t capacity) noexcept
 : _engine_context(engine_context) {
	try {
		_meshing_tasks.resize(capacity);
	} catch(const std::exception&) {
		_engine_context.error |= Error::CPU_ALLOCATION_FAILED;
		return;
	}

    try {
		threads_count = 1;
		_threads.reserve(threads_count);
		const std::size_t mesh_size = 
			_engine_context.chunk_max_current_mesh_size/sizeof(Vertex);
        for (std::size_t i{ 0U }; i < threads_count; ++i) {
			auto& t = _threads.emplace_back();
			for (auto& staging_buffer : t.staging_buffers)
				staging_buffer.resize(mesh_size, {0});

			t.jthread = std::jthread(&CpuMesher::thread, this, i);
        }
    } catch(const std::exception&) {
        _engine_context.error |= Error::CPU_MESHING_ENGINE_THREAD_ALLOCATION_FAILED;
        _done = true;
    }
}

void CpuMesher::updateLimits() noexcept {
	while (_ready_counter > 0) {}

	overflowed = false;

	cond_overflowed.notify_all();
}

void CpuMesher::thread(std::size_t thread_index) noexcept {
	if (_done)
		return;

	++_ready_counter;

	auto& self = _threads[thread_index];

	while (!_done) {
		auto* use_flag = &self.staging_buffer_in_use_flags[self.current_buffer];
		while (use_flag->load(std::memory_order_acquire)) {
			self.current_buffer = (self.current_buffer + 1) % self.staging_buffers.size();
			use_flag = &self.staging_buffer_in_use_flags[self.current_buffer];
		}

		MeshingTask meshing_task;
		if (_meshing_tasks.read(meshing_task)) {
			auto& staging_buffer = self.staging_buffers[self.current_buffer];
			self.current_buffer = (self.current_buffer + 1) % self.staging_buffers.size();

			auto value = greedyMeshing(staging_buffer,
						meshing_task.chunk_position, meshing_task.voxel_data);

			value.staging_buffer_in_use_flag = use_flag;
#ifdef ENGINE_TEST	
			value.cmd_timer_meshing.stop();
#endif
			std::lock_guard lock_guard(m_overflowed);
			// TODO: optimize this
			if (overflowed) {
				// NOTE: return back the command it will
				// be served again after limits are updated
				_meshing_tasks.write(std::move(meshing_task));
			} else {
				if (value.overflow_flag) {
					overflowed = true;
				} else {
					value.staging_buffer_in_use_flag->store(true, std::memory_order_relaxed);
				}
				meshing_task.promise.set_value(std::move(value));
			}
		} else {
            std::this_thread::yield();
		}

		if (overflowed) {
			--_ready_counter;
			std::unique_lock lk(m_overflowed);
			cond_overflowed.wait(lk);

			const std::size_t mesh_size = 
				_engine_context.chunk_max_current_mesh_size/sizeof(Vertex);
			for (auto& buffer : self.staging_buffers) {
				try { buffer.resize(mesh_size); }
				catch (const std::exception&) {
					_engine_context.error |= Error::CPU_ALLOCATION_FAILED;
					_done = true;
				}
			}
			self.current_buffer = 0;

			++_ready_counter;
		}
	}

}

std::future<CpuMesher::Promise> CpuMesher::mesh(vmath::Vec3f32 chunk_position, std::span<const vmath::u16> voxel_data) noexcept {
    // can throw but will never happen in practice
	std::promise<Promise> promise;
    std::future<Promise> result(promise.get_future());
    
    /// will never return false since size == max chunks count
    _meshing_tasks.write(std::move(promise), chunk_position, voxel_data);

    return result;
}

//#define GREEDY_MESHING_DONT_USE_FUTURES

CpuMesher::Promise CpuMesher::greedyMeshing(
		std::vector<Vertex>& out, 
		vmath::Vec3f32 chunk_position,
		std::span<const vmath::u16> voxel_data) noexcept {
	CpuMesher::Promise result;

	const std::size_t mesh_size = 
		_engine_context.chunk_max_current_mesh_size/sizeof(Vertex);
	const std::size_t submesh_size = 
		_engine_context.chunk_max_current_submesh_size/sizeof(Vertex);
#ifndef GREEDY_MESHING_DONT_USE_FUTURES
	std::array<std::future<GreedyMeshingPromise>, 6> futures;
#endif
#ifdef ENGINE_TEST
	result.cmd_timer_meshing.start();
	result.cmd_timer_real.start();
#endif
	for (u32 face{ 0 }; face < 6; ++face) {
		auto axis = face / 2;
		const Vec3u32 logical_indices {
			(axis + 1) % 3,
			(axis + 2) % 3,
			axis
		};
		const Vec3u32 real_indices {
			(2 + axis * 2) % 3,
			(0 + axis * 2) % 3,
			(1 + axis * 2) % 3
		};
		const Vec3i32 logical_extent {
			_engine_context.chunk_size[logical_indices[0]],
			_engine_context.chunk_size[logical_indices[1]],
			_engine_context.chunk_size[logical_indices[2]]
		};
		
		const i32 edge_value = (face % 2) == 1 ? 0 : logical_extent[2] - 1;
		const i32 polarity   = (face % 2) == 1 ?-1 : 1;

		const Vec2u32 squashed_extent_logical_indices = {
			static_cast<u32>(axis == 0),
			static_cast<u32>(axis != 0)
		};

    	const i32 plane_size = logical_extent[0] * logical_extent[1];

		std::span<Vertex> out_subregion(out.data() + face * submesh_size, submesh_size);

#ifdef GREEDY_MESHING_DONT_USE_FUTURES
		const auto val = CpuMesher::greedyMeshingFace(
			out_subregion, chunk_position, voxel_data,
			GreedyMeshingFaceDescriptor{
				static_cast<Face>(face),
				axis,
				logical_indices,
				real_indices,
				logical_extent,
				edge_value,
				polarity,
				squashed_extent_logical_indices,
				plane_size,
				submesh_size
			});
		result.written_quads[face] = val.written_quads;
		result.overflow_flag = result.overflow_flag || val.overflow_flag;
#else
		futures[face] = std::async(
			&CpuMesher::greedyMeshingFace, this,
			out_subregion, chunk_position, voxel_data,
			GreedyMeshingFaceDescriptor{
				static_cast<Face>(face),
				axis,
				logical_indices,
				real_indices,
				logical_extent,
				edge_value,
				polarity,
				squashed_extent_logical_indices,
				plane_size,
				submesh_size
			}
		);
#endif
	}

#ifndef GREEDY_MESHING_DONT_USE_FUTURES
	for (u32 face{ 0 }; face < 6; ++face) {
		const auto future_val = futures[face].get();
		result.written_quads[face] = future_val.written_quads;
		result.overflow_flag = result.overflow_flag || future_val.overflow_flag;
	}
#endif
	result.staging_buffer_ptr = std::span<Vertex>(out.data(), mesh_size);

	return result;
}

CpuMesher::GreedyMeshingPromise CpuMesher::greedyMeshingFace(std::span<Vertex> out, 
		vmath::Vec3f32 chunk_position,
		std::span<const vmath::u16> voxel_data,
		GreedyMeshingFaceDescriptor desc
) noexcept {

	static constexpr Vec3f32 VERTICES[8] = {
		{0.f, 0.f, 0.f}, // 0
		{0.f, 0.f, 1.f}, // 1
		{0.f, 1.f, 0.f}, // 2 
		{0.f, 1.f, 1.f}, // 3
		{1.f, 0.f, 0.f}, // 4
		{1.f, 0.f, 1.f}, // 5
		{1.f, 1.f, 0.f}, // 6
		{1.f, 1.f, 1.f}  // 7
	};
	static constexpr Vec2f32 TEX_COORDS[4] = {
		{0.f, 0.f},
		{1.f, 0.f},
		{1.f, 1.f},
		{0.f, 1.f}
	};
	static constexpr std::size_t QUADS[6][4] = {
		{ 5, 4, 6, 7 },
		{ 0, 1, 3, 2 },
		{ 2, 3, 7, 6 },
		{ 4, 5, 1, 0 },
		{ 1, 5, 7, 3 },
		{ 4, 0, 2, 6 }
	};
	
	//std::array<bool, 64*64> states;
	//std::fill(states.begin(), states.end(), false);
	std::bitset<64 * 64> states(0);
	GreedyMeshingPromise result{};
	i32 vertices_writer{ 0 };
	for (i32 slice{ 0 }; slice < desc.logical_extent[2]; ++slice) {
		
	Vec3i32 i{ 0, 0, slice };
	bool any_visible{ false };

	for (;i[1] < desc.logical_extent[1]; ++i[1]) {
		for (i[0] = 0; i[0] < desc.logical_extent[0]; ++i[0]) {
			const std::size_t voxel_index = 
				i[desc.real_indices[0]] +
				i[desc.real_indices[1]] * desc.logical_extent[0] +
				i[desc.real_indices[2]] * desc.plane_size;
			
			if (voxel_data[voxel_index] == 0)
				continue;

			if (i[2] == desc.edge_value) {
				states[i[0] + i[1] * desc.logical_extent[0]] = true;
				any_visible = true;			
				continue;
			}

			i[2] += desc.polarity; // BEGIN

			const std::size_t neighbouring_voxel_index = 
				i[desc.real_indices[0]] +
				i[desc.real_indices[1]] * desc.logical_extent[0] +
				i[desc.real_indices[2]] * desc.plane_size;

			if (voxel_data[neighbouring_voxel_index] == 0) {
				states[i[0] + i[1] * desc.logical_extent[0]] = true;
				any_visible = true;			
			}

			i[2] -= desc.polarity; // END
		}
	}

	if (!any_visible)
		continue;

	for (i[1] = 0; i[1] < desc.logical_extent[1]; ++i[1]) {
		for (i[0] = 0; i[0] < desc.logical_extent[0];) {
			if (!states[i[0] + i[1] * desc.logical_extent[0]]) {
				++i[0];
				continue;
			}
			const std::size_t voxel_index = 
				i[desc.real_indices[0]] +
				i[desc.real_indices[1]] * desc.logical_extent[0] +
				i[desc.real_indices[2]] * desc.plane_size;
			const auto voxel_value = voxel_data[voxel_index];

			Vec3i32 mesh_region = Vec3i32(1, 0, 0);

			for (;i[0] + mesh_region[0] < desc.logical_extent[0]; ++mesh_region[0]) {
				const std::size_t state_index =
					(i[0] + mesh_region[0]) + i[1] * desc.logical_extent[0];

				if (!states[state_index])
					break;

				const std::size_t next_voxel_index =
					(i[desc.real_indices[0]] + mesh_region[desc.real_indices[0]]) + 
					(i[desc.real_indices[1]] + mesh_region[desc.real_indices[1]]) * desc.logical_extent[0] + 
					(i[desc.real_indices[2]] + mesh_region[desc.real_indices[2]]) * desc.plane_size;
				const auto next_voxel_value = voxel_data[next_voxel_index];
				
				if (voxel_value != next_voxel_value)
					break;
			}

			const i32 mesh_region_x = mesh_region[0];
			mesh_region[0] = 0;

			for (mesh_region[1] = 1; i[1] + mesh_region[1] < desc.logical_extent[1]; ++mesh_region[1]) {
				Vec3i32 tmp_mesh_region{ 0, 0, 0 };
				bool outer_break{ false };
				for (;i[0] + tmp_mesh_region[0] < i[0] + mesh_region_x; ++tmp_mesh_region[0]) {
					const std::size_t state_index =
						(i[0] + tmp_mesh_region[0]) + (i[1] + mesh_region[1]) * desc.logical_extent[0];
					if (!states[state_index]) {
						outer_break = true;
						break;
					}

					const std::size_t next_voxel_index =
						(i[desc.real_indices[0]] + mesh_region[desc.real_indices[0]] + tmp_mesh_region[desc.real_indices[0]]) + 
						(i[desc.real_indices[1]] + mesh_region[desc.real_indices[1]] + tmp_mesh_region[desc.real_indices[1]]) * desc.logical_extent[0] + 
						(i[desc.real_indices[2]] + mesh_region[desc.real_indices[2]] + tmp_mesh_region[desc.real_indices[2]]) * desc.plane_size;
					const auto next_voxel_value = voxel_data[next_voxel_index];

					if (voxel_value != next_voxel_value) {
						outer_break = true;
						break;
					}
				}
				if (outer_break)
					break;
			}

			mesh_region[0] = mesh_region_x;

			Vec3f32 region_extent{ 0.f, 0.f, 0.f };
			region_extent[desc.logical_indices[2]] = 1.f;
			region_extent[desc.logical_indices[1]] = static_cast<f32>(mesh_region[1]);
			region_extent[desc.logical_indices[0]] = static_cast<f32>(mesh_region[0]);

			Vec2f32 squashed_region_extent {
				static_cast<f32>(mesh_region[desc.squashed_extent_logical_indices[0]]),
				static_cast<f32>(mesh_region[desc.squashed_extent_logical_indices[1]])
			};

			Vec3f32 region_offset = chunk_position;
			region_offset[desc.logical_indices[2]] += static_cast<float>(i[2]);
			region_offset[desc.logical_indices[1]] += static_cast<float>(i[1]);
			region_offset[desc.logical_indices[0]] += static_cast<float>(i[0]);
			

			if (vertices_writer + 4 > desc.max_submesh_size) {
				result.overflow_flag = true;
			} else {
				const auto voxel_value_encoded = static_cast<f32>(6 * (voxel_value - 1)) + static_cast<f32>(desc.face);
				Vertex v[4];
				v[0].position = Vec3f32::add(Vec3f32::mul(region_extent, VERTICES[QUADS[desc.face][0]]), region_offset);
				v[0].texcoord[0] = squashed_region_extent[0] * TEX_COORDS[0][0];
				v[0].texcoord[1] = squashed_region_extent[1] * TEX_COORDS[0][1];
				v[0].texcoord[2] = voxel_value_encoded;

				v[1].position = Vec3f32::add(Vec3f32::mul(region_extent, VERTICES[QUADS[desc.face][1]]), region_offset);
				v[1].texcoord[0] = squashed_region_extent[0] * TEX_COORDS[1][0];
				v[1].texcoord[1] = squashed_region_extent[1] * TEX_COORDS[1][1];
				v[1].texcoord[2] = voxel_value_encoded;

				v[2].position = Vec3f32::add(Vec3f32::mul(region_extent, VERTICES[QUADS[desc.face][2]]), region_offset);
				v[2].texcoord[0] = squashed_region_extent[0] * TEX_COORDS[2][0];
				v[2].texcoord[1] = squashed_region_extent[1] * TEX_COORDS[2][1];
				v[2].texcoord[2] = voxel_value_encoded;

				v[3].position = Vec3f32::add(Vec3f32::mul(region_extent, VERTICES[QUADS[desc.face][3]]), region_offset);
				v[3].texcoord[0] = squashed_region_extent[0] * TEX_COORDS[3][0];
				v[3].texcoord[1] = squashed_region_extent[1] * TEX_COORDS[3][1];
				v[3].texcoord[2] = voxel_value_encoded;
			
				memcpy(out.data() + vertices_writer, v, sizeof(v));
			}

			vertices_writer += 4;
			for (int y = i[1]; y < i[1] + mesh_region[1]; ++y)
				for (int x = i[0]; x < i[0] + mesh_region[0]; ++x)
					states[x + y * desc.logical_extent[0]] = false;

			i[0] += mesh_region[0];
		}
	}
	}
	result.written_quads = vertices_writer/4;
	return result;
}
