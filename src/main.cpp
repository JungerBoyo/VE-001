#include <glad/glad.h>

#include <gl_context.h>
#include <window/window.h>
#include <vmath/vmath.h>

#include <voxel_terrain_generator/voxel_terrain_generator.h>
#include <engine/meshing_engine.h>
#include <engine/chunk_mesh_pool.h>
#include <engine/cubemap.h>
#include <engine/texture_rgba8_array.h>
#include <sandbox_utils/camera.h>
#include <gl_boilerplate/shader.h>

#include <cstring>
#include <numbers>

using namespace ve001;
using namespace vmath;

constexpr u32 POSITION_ATTRIB_INDEX{ 0U };
constexpr u32 TEXCOORD_ATTRIB_INDEX{ 1U };

constexpr u32 CONFIG_2D_TEX_ARRAY_BINDING{ 1U };
constexpr u32 CONFIG_MVP_UBO_BINDING{ 0 };

struct Vertex {
    Vec3f32 position;
    Vec3f32 texcoord;
};

void setVertexLayout(u32 vao, u32 vbo) {
    const u32 vertex_attrib_binding = 0U;

    glEnableVertexArrayAttrib(vao, POSITION_ATTRIB_INDEX);
    glEnableVertexArrayAttrib(vao, TEXCOORD_ATTRIB_INDEX);

    glVertexArrayAttribFormat(vao, POSITION_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glVertexArrayAttribFormat(vao, TEXCOORD_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, texcoord));
    
    glVertexArrayAttribBinding(vao, POSITION_ATTRIB_INDEX, vertex_attrib_binding);
    glVertexArrayAttribBinding(vao, TEXCOORD_ATTRIB_INDEX, vertex_attrib_binding);

    glVertexArrayBindingDivisor(vao, vertex_attrib_binding, 0);

    glVertexArrayVertexBuffer(vao, vertex_attrib_binding, vbo, 0, sizeof(Vertex));
}

constexpr Vec3f32 genPosition(u8 bin, Vec3f32 scale) { switch (bin) {
    case 0: return Vec3f32(0.F,         0.F,        0.F);       // --- 
    case 1: return Vec3f32(0.F,         0.F,        scale[2]);  // --+
    case 2: return Vec3f32(0.F,         scale[1],   0.F);       // -+-
    case 3: return Vec3f32(0.F,         scale[1],   scale[2]);  // -++
    case 4: return Vec3f32(scale[0],    0.F,        0.F);       // +--
    case 5: return Vec3f32(scale[0],    0.F,        scale[2]);  // +-+
    case 6: return Vec3f32(scale[0],    scale[1],   0.F);       // ++-
    case 7: return Vec3f32(scale[0],    scale[1],   scale[2]);  // +++

    default: return Vec3f32(0.F, 0.F, 0.F);
}}

std::array<Vertex, 6> genFace(u8 face, Vec3f32 offset, Vec3f32 scale, Vec2f32 face_scale) {
    std::array<Vertex, 6> faces;
    switch (face) {
    case 0: //POS_X: 
        faces[0].position = Vec3f32::add(offset, genPosition(5U, scale)); 
        faces[1].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(6U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(5U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(6U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(7U, scale));

    break;
    case 1: //NEG_X: 
        faces[0].position = Vec3f32::add(offset, genPosition(0U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(0U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(3U, scale));
    break;
    case 2: //POS_Y: 
        faces[0].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(3U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(7U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(7U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(6U, scale));
    break;
    case 3: //NEG_Y: 
        faces[0].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(5U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(0U, scale));
    break;
    case 4: //POS_Z: 
        faces[0].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(5U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(7U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(1U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(7U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(3U, scale));
    break;
    case 5: //NEG_Z: 
        faces[0].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[1].position = Vec3f32::add(offset, genPosition(0U, scale));
        faces[2].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[3].position = Vec3f32::add(offset, genPosition(4U, scale));
        faces[4].position = Vec3f32::add(offset, genPosition(2U, scale));
        faces[5].position = Vec3f32::add(offset, genPosition(6U, scale));
    break;
    }

    faces[0].texcoord = Vec3f32(0.F,            0.F,            static_cast<f32>(face));
    faces[1].texcoord = Vec3f32(face_scale[0],  0.F,            static_cast<f32>(face));
    faces[2].texcoord = Vec3f32(face_scale[0],  face_scale[1],  static_cast<f32>(face));
    faces[3].texcoord = Vec3f32(0.F,            0.F,            static_cast<f32>(face));
    faces[4].texcoord = Vec3f32(face_scale[0],  face_scale[1],  static_cast<f32>(face));
    faces[5].texcoord = Vec3f32(0.F,            face_scale[1],  static_cast<f32>(face));

    return faces;
}

TextureRGBA8Array createCubeMapTextureArray(const std::filesystem::path& img_path) {
    CubeMap cubemap{};
    CubeMap::load(cubemap, img_path);

    auto texture_rgba8_array = TextureRGBA8Array::init(cubemap.width, cubemap.height, 6);
    i32 i{ 0 }; 
    for (const auto& face : cubemap.faces) {
        texture_rgba8_array.writeData(static_cast<const void*>(face.data()), i++);
    }
    
    return texture_rgba8_array;
}

int main() {
    if (!ve001::window.init("demo", 640, 480, nullptr)) {
        return 1;
    }

    ve001::glInit();
    // ve001::setGLDebugCallback([](u32, u32, u32 id, u32, int, const char *message, const void *) {
	// 	fmt::print("[GLAD]{}:{}", id, message);
	// });

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

    ve001::ChunkMeshPool chunk_mesh_pool{};
    chunk_mesh_pool.init({
        .chunk_dimensions = Vec3i32(16, 256, 16),
        .max_chunks = 9,
        .vertex_size = sizeof(Vertex),
        .vertex_layout_config = setVertexLayout
    });

    ve001::MeshingEngine meshing_engine({
       .executor = MeshingEngine::Config::Executor::CPU,
       .chunk_size = Vec3i32(16, 256, 16) 
    });

    ve001::VoxelTerrainGenerator terrain_generator({
        .terrain_size = Vec3i32(16, 256, 16),
        .terrain_density = 4U,
        .noise_type = VoxelTerrainGenerator::Config::NoiseType::PERLIN,
        .quantize_values = 1U,
        .quantized_value_size = 1U,
        .seed = 0x42422424
    });
    terrain_generator.init();

    std::vector<u8> noise(16 * 256 * 16, 0U);
    terrain_generator.next(static_cast<void*>(noise.data()), 0U, 0U, Vec3i32(0));
    
    u32 chunk_id{ 0U };
    chunk_mesh_pool.allocateEmptyChunk(chunk_id, Vec3f32(0.F));

    void* chunk_dst_ptr{ nullptr };
    chunk_mesh_pool.aquireChunkWritePtr(chunk_id, chunk_dst_ptr);

    const auto per_face_vertex_count = meshing_engine.mesh(
        chunk_dst_ptr, 0U, sizeof(Vertex), chunk_mesh_pool.submeshStride(),
        Vec3f32(0.F), 
        [&noise](i32 x, i32 y, i32 z) {
            const auto index = static_cast<std::size_t>(x + y * 16 + z * 16 * 256);
            return noise[index] > 0U;
        },
        [&](void* dst, MeshingEngine::MeshedRegionDescriptor descriptor) {
            const auto region_offset = Vec3f32::cast(descriptor.region_offset);
            const auto region_extent = Vec3f32::cast(descriptor.region_extent);
            const auto squashed_region_offset = Vec2f32::cast(descriptor.squashed_region_extent);

            const auto face = genFace(
                descriptor.face,
                region_offset,
                region_extent,
                squashed_region_offset
            );

            std::memcpy(dst, static_cast<const void*>(face.data()), face.size() * sizeof(Vertex));
        }
    );

    chunk_mesh_pool.freeChunkWritePtr(chunk_id);

    chunk_mesh_pool.updateChunkSubmeshVertexCounts(chunk_id, per_face_vertex_count);

    auto texture_rgba8_array = createCubeMapTextureArray(
        "/home/regu/codium_repos/VE-001/assets/textures/mcgrasstexture.png"
    );

    Shader shader{};
    shader.attach(
        "/home/regu/codium_repos/VE-001/shaders/bin/basic_shader/vert.spv", 
        "/home/regu/codium_repos/VE-001/shaders/bin/basic_shader/frag.spv"
    );
    shader.bind();
    
    texture_rgba8_array.bind(CONFIG_2D_TEX_ARRAY_BINDING);

    u32 ubo{ 0U };
    glCreateBuffers(1, &ubo);
    glNamedBufferStorage(ubo, sizeof(Mat4f32), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, CONFIG_MVP_UBO_BINDING, ubo);

    Camera camera{};

    camera.move({0.F, 0.F, 10.F});

    glClearColor(.22F, .61F, .78F, 1.F);

    f32 prev_frame_time{ 0.F };
    while(!window.shouldClose()) {
        const auto [window_width, window_height] = window.size();

        const auto frame_time = window.time();
        const auto timestep = frame_time - prev_frame_time;
        prev_frame_time = frame_time;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, window_width, window_height);

        const auto proj_mat = misc<f32>::symmetricPerspectiveProjection(
            .25F * std::numbers::pi_v<f32>, .1F, 1000.F, 
            static_cast<f32>(window_width), static_cast<f32>(window_height)
        );

        const auto mvp = Mat4f32::mul(proj_mat, camera.lookAt());
        glNamedBufferSubData(ubo, 0, sizeof(Mat4f32), static_cast<const void*>(&mvp));

        chunk_mesh_pool.drawAll();

        window.swapBuffers();
        window.pollEvents();
    }

    glDeleteBuffers(1, &ubo);
    shader.deinit();
    chunk_mesh_pool.deinit();
    texture_rgba8_array.deinit();
    window.deinit();

    return 0;
}