#include "engine.h"

using namespace vmath;
using namespace ve001;

static bool frustumCullingUnaryOp(
	Face orientation, 
	Vec3f32 position,
	Vec3f32 chunk_size,
	f32 z_near, 
	f32 z_far, 
	f32 x_near,
	f32 y_near,
	Mat4f32 view_matrix
);


Engine::Engine(Config config) : _engine_context(EngineContext{
		.chunk_size = config.chunk_size,
		.half_chunk_size = Vec3i32::divScalar(config.chunk_size, 2),
		.chunk_size_1D = static_cast<u64>(config.chunk_size[0]) * static_cast<u64>(config.chunk_size[1]) * static_cast<u64>(config.chunk_size[2]),
		.chunk_voxel_data_size  = static_cast<u64>(config.chunk_size[0]) * static_cast<u64>(config.chunk_size[1]) * static_cast<u64>(config.chunk_size[2]) * sizeof(u16),
		.chunk_max_possible_submesh_indices_size = static_cast<u64>(config.chunk_size[0]) * static_cast<u64>(config.chunk_size[1]) * static_cast<u64>(config.chunk_size[2]) * sizeof(u32) * static_cast<u64>(36/6/2),
		.chunk_max_possible_mesh_size    =  (static_cast<u64>(config.chunk_size[0]) * static_cast<u64>(config.chunk_size[1]) * static_cast<u64>(config.chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(24/2)),
		.chunk_max_possible_submesh_size = ((static_cast<u64>(config.chunk_size[0]) * static_cast<u64>(config.chunk_size[1]) * static_cast<u64>(config.chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(24/2))/6),
		.chunk_max_current_mesh_size    = config.chunk_pool_growth_coefficient == 0.F ? 
			(static_cast<u64>(config.chunk_size[0]) * static_cast<u64>(config.chunk_size[1]) * static_cast<u64>(config.chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(24/2)) :
			sizeof(Vertex) * static_cast<u64>(24),
		.chunk_max_current_submesh_size = config.chunk_pool_growth_coefficient == 0.F ? 
			((static_cast<u64>(config.chunk_size[0]) * static_cast<u64>(config.chunk_size[1]) * static_cast<u64>(config.chunk_size[2]) * sizeof(Vertex) * static_cast<u64>(24/2))/6) :
			sizeof(Vertex) * static_cast<u64>(24/6),
		.chunk_pool_growth_coefficient = config.chunk_pool_growth_coefficient,
		.meshing_axis_progress_step = config.meshing_shader_local_group_size,
		.meshing_shader_src_path = config.meshing_shader_src_path,
		.meshing_shader_bin_path = config.meshing_shader_bin_path 	
  	}),
  	_world_grid(_engine_context, config.world_size, config.initial_position, config.chunk_data_streamer_threads_count, std::move(config.chunk_data_generator))
{}

void Engine::init() {
    _world_grid.init();
}
void Engine::deinit() {
    _world_grid.deinit();
}

void Engine::applyFrustumCullingPartition(
	bool use_last_partition, 
	f32 z_near, 
	f32 z_far, 
	f32 x_near,
	f32 y_near,
	Mat4f32 view_matrix) {
		
	_world_grid._chunk_pool.partitionDrawCommands(
		frustumCullingUnaryOp,
		use_last_partition,
		Vec3f32::cast(_engine_context.half_chunk_size),
		z_near, 
		z_far, 
		x_near,
		y_near,
		view_matrix
	);
}

void Engine::updateCameraPosition(Vec3f32 position) {
	_world_grid.update(position);
}
bool Engine::pollChunksUpdates() {
	return _world_grid._chunk_pool.poll();
}
void Engine::updateDrawState() {
	_world_grid._chunk_pool.update(partitioning);
}
void Engine::draw() {
	_world_grid._chunk_pool.drawAll(partitioning);
}

bool frustumCullingUnaryOp(
	Face orientation, 
	Vec3f32 position,
	Vec3f32 half_chunk_size,	
	f32 z_near, 
	f32 z_far, 
	f32 x_near,
	f32 y_near,
	Mat4f32 view_matrix
) {
	// center position in view space
	const auto position_in_view_space = Mat4f32::mulVec(view_matrix, {position[0], position[1], position[2], 1.F});

	const auto chunk_min = Vec3f32::add(position, half_chunk_size);
	const auto chunk_max = Vec3f32::sub(position, half_chunk_size);

	const std::array<Vec4f32, 4> corners{{
		Mat4f32::mulVec(view_matrix, {chunk_min[0], chunk_min[1], chunk_min[2], 1.F}),
		Mat4f32::mulVec(view_matrix, {chunk_max[0], chunk_min[1], chunk_min[2], 1.F}),
		Mat4f32::mulVec(view_matrix, {chunk_min[0], chunk_max[1], chunk_min[2], 1.F}),
		Mat4f32::mulVec(view_matrix, {chunk_min[0], chunk_min[1], chunk_max[2], 1.F}),
	}};

	const std::array<Vec3f32, 3> axes{{
		Vec3f32::normalize(Vec3f32::sub({corners[1][0], corners[1][1], corners[1][2]}, {corners[0][0], corners[0][1], corners[0][2]})),		
		Vec3f32::normalize(Vec3f32::sub({corners[2][0], corners[2][1], corners[2][2]}, {corners[0][0], corners[0][1], corners[0][2]})),		
		Vec3f32::normalize(Vec3f32::sub({corners[3][0], corners[3][1], corners[3][2]}, {corners[0][0], corners[0][1], corners[0][2]})),		
	}};

	// determine if chunk is behind z_near plane or beyond z_far plane
	{
	//const Vec3f32 projection_line{ 0.F, 0.F, 1.F }; implicit

	const auto projected_center = position_in_view_space[2];

	const auto radius =
		fabsf(axes[0][2]) * (half_chunk_size[0]) +
		fabsf(axes[1][2]) * (half_chunk_size[1]) +
		fabsf(axes[2][2]) * (half_chunk_size[2]);

	const auto min = projected_center - radius;
	const auto max = projected_center + radius;

	if (min > z_near || max < z_far) {
		return false;
	}
	}	
	// determine if chunk is beyond the rest of the frustum planes
	{
	const Vec3f32 projection_lines[4] {
		{-z_near, 0.F, x_near}, // left plane
		{ z_near, 0.F, x_near}, // right plane
		{ 0.F,-z_near, y_near}, // top plane
		{ 0.F, z_near, y_near}  // bottom plane
	};
	for (std::size_t i{ 0U }; i < 4U; ++i) {
		const auto projected_center = Vec3f32::dot(projection_lines[i], { position_in_view_space[0], position_in_view_space[1], position_in_view_space[2] });
		const auto radius = 
			fabsf(Vec3f32::dot(projection_lines[i], axes[0])) * half_chunk_size[0] +
			fabsf(Vec3f32::dot(projection_lines[i], axes[1])) * half_chunk_size[1] +
			fabsf(Vec3f32::dot(projection_lines[i], axes[2])) * half_chunk_size[2];

		const auto min = projected_center - radius;
		const auto max = projected_center + radius;

		const auto p = x_near * fabsf(projection_lines[i][0]) + y_near * fabsf(projection_lines[i][1]);

		auto tau_0 = z_near * projection_lines[i][2] - p;
		auto tau_1 = z_near * projection_lines[i][2] + p;

		if (tau_0 < 0.F) {
			tau_0 *= (z_far/z_near);
		}
		if (tau_1 > 0.F) {
			tau_1 *= (z_far/z_near);
		}

		if (min > tau_1 || max < tau_0) {
			return false;
		}
	}
	}
	// determine if frustum is inside of the chunk
	for (std::size_t i{ 0U }; i < 3U; ++i) {
		const auto projected_center = Vec3f32::dot(axes[i], { position_in_view_space[0], position_in_view_space[1], position_in_view_space[2] });
		const auto radius = half_chunk_size[i];

		const auto min = projected_center - radius;
		const auto max = projected_center + radius;

		const auto p = x_near * fabsf(axes[i][0]) + y_near * fabsf(axes[i][1]);

		auto tau_0 = z_near * axes[i][2] - p;
		auto tau_1 = z_near * axes[i][2] + p;

		if (tau_0 < 0.F) {
			tau_0 *= (z_far/z_near);
		}
		if (tau_1 > 0.F) {
			tau_1 *= (z_far/z_near);
		}

		if (min > tau_1 || max < tau_0) {
			return false;
		}
	}
	// check the cross products of the edges

	// cross({1,0,0}, axes[0,1,2])
	for (std::size_t i{ 0U }; i < 3U; ++i) {
		// cross product always the same implicit computation
		const Vec3f32 projection_line{ 0.F, -axes[i][2], axes[i][1] };

		const auto projected_center = 
			projection_line[1] * position_in_view_space[1] +
			projection_line[2] * position_in_view_space[2];

		const auto radius = 
			fabsf(Vec3f32::dot(projection_line, axes[0])) * half_chunk_size[0] + 
			fabsf(Vec3f32::dot(projection_line, axes[1])) * half_chunk_size[1] + 
			fabsf(Vec3f32::dot(projection_line, axes[2])) * half_chunk_size[0];

		const auto min = projected_center - radius;
		const auto max = projected_center + radius;

		const auto p = y_near * fabsf(projection_line[1]);
		auto tau_0 = z_near * projection_line[2] - p;
		auto tau_1 = z_near * projection_line[2] + p;
		if (tau_0 < 0.F) {
			tau_0 *= (z_far/z_near);
		}
		if (tau_1 > 0.F) {
			tau_1 *= (z_far/z_near);
		}

		if (min > tau_1 || max < tau_0) {
			return false;
		}
	}

	// cross({0,1,0}, axes[0,1,2])
	for (std::size_t i{ 0U }; i < 3U; ++i) {
		// cross product always the same implicit computation
		const Vec3f32 projection_line{ axes[i][2], 0.F, -axes[i][0] };

		const auto projected_center = 
			projection_line[0] * position_in_view_space[0] +
			projection_line[2] * position_in_view_space[2];

		const auto radius = 
			fabsf(Vec3f32::dot(projection_line, axes[0])) * half_chunk_size[0] + 
			fabsf(Vec3f32::dot(projection_line, axes[1])) * half_chunk_size[1] + 
			fabsf(Vec3f32::dot(projection_line, axes[2])) * half_chunk_size[0];

		const auto min = projected_center - radius;
		const auto max = projected_center + radius;

		const auto p = x_near * fabsf(projection_line[0]);
		auto tau_0 = z_near * projection_line[2] - p;
		auto tau_1 = z_near * projection_line[2] + p;
		if (tau_0 < 0.F) {
			tau_0 *= (z_far/z_near);
		}
		if (tau_1 > 0.F) {
			tau_1 *= (z_far/z_near);
		}

		if (min > tau_1 || max < tau_0) {
			return false;
		}
	}

	// cross(frustum edges, axes[0,1,2])
	for (std::size_t i{ 0U }; i < 3; ++i) {
		const Vec3f32 projection_lines[4] = {
			vmath::cross({-x_near, 0.F, z_near }, axes[i]), // Left Plane
            vmath::cross({ x_near, 0.F, z_near }, axes[i]), // Right plane
            vmath::cross({ 0.F, y_near, z_near }, axes[i]), // Top plane
            vmath::cross({ 0.F,-y_near, z_near }, axes[i]) // Bottom plane
		};

		for (std::size_t j{ 0U }; j < 4; ++j) {
			const auto MoX = fabsf(projection_lines[j][0]);
			const auto MoY = fabsf(projection_lines[j][1]);
			const auto MoZ = projection_lines[j][2];

			constexpr f32 epsilon = 1e-4;
			if (MoX < epsilon && MoY < epsilon && fabsf(MoZ) < epsilon) continue;

			const auto projected_center = Vec3f32::dot(projection_lines[j], {position_in_view_space[0], position_in_view_space[1], position_in_view_space[2]});

			const auto radius =
				fabsf(Vec3f32::dot(projection_lines[j], axes[0])) * half_chunk_size[0] + 
				fabsf(Vec3f32::dot(projection_lines[j], axes[1])) * half_chunk_size[1] + 
				fabsf(Vec3f32::dot(projection_lines[j], axes[2])) * half_chunk_size[2];

			const auto min = projected_center - radius;
			const auto max = projected_center + radius;

			// Frustum projection
			const auto p = x_near * MoX + y_near * MoY;
			auto tau_0 = z_near * MoZ - p;
			auto tau_1 = z_near * MoZ + p;
			if (tau_0 < 0.F) {
				tau_0 *= (z_far/z_near);
			}
			if (tau_1 > 0.F) {
				tau_1 *= (z_far/z_near);
			}

			if (min > tau_1 || max < tau_0) {
				return false;
			}
		}
	}
	return true;
}