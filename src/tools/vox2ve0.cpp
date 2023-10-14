#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <fmt/color.h>
#include <zstd.h>

#include <vmath/vmath.h>

#include <vector>
#include <filesystem>
#include <fstream>
#include <exception>
#include <algorithm>

namespace std {
    namespace fs = filesystem;
}

using namespace vmath;

using Vec4u8 = Vec4<u8>;

struct VoxChunkHeader {
    char id[4];
    u32 content_size;
    u32 children_size;
};

void printVoxChunkHeader(VoxChunkHeader header) {
    spdlog::info("Header :: {}{}{}{}, Content size :: {}, Children size :: {}", 
        header.id[0], 
        header.id[1], 
        header.id[2], 
        header.id[3],
        header.content_size,
        header.children_size
    );
}
void printVoxChunkUnknownWarn(VoxChunkHeader header) {
    spdlog::warn("Unknown chunk type \"{}{}{}{}\", ignoring...", 
        header.id[0], 
        header.id[1], 
        header.id[2], 
        header.id[3]
    );
}

constexpr u32 chunkId2u32(const char* id) {
    return 
        (static_cast<u32>(id[3]) << 24) |
        (static_cast<u32>(id[2]) << 16) |
        (static_cast<u32>(id[1]) <<  8) |
        (static_cast<u32>(id[0]) <<  0);
}

enum : u32 {
    CHUNK_ID_SIZE = chunkId2u32("SIZE"),
    CHUNK_ID_XYZI = chunkId2u32("XYZI"),
    CHUNK_ID_RGBA = chunkId2u32("RGBA")
};

struct VoxChunk {
    Vec3u32 SIZE;
    std::vector<Vec4u8> XYZIs;
};

std::optional<VoxChunkHeader> parseVoxChunkHeader(std::ifstream& stream) {
    VoxChunkHeader result{};
    const auto size = stream.readsome(reinterpret_cast<char*>(&result), sizeof(VoxChunkHeader));
    if (size != sizeof(VoxChunkHeader)) {
        return std::nullopt;
    }
    return result;
}

struct ParsedVoxFileData {
    std::vector<VoxChunk> vox_chunks;
    std::array<Vec4u8, 256> palette;
};

std::vector<u8> fetchActiveColorIndicesFromPalette(const ParsedVoxFileData& parsed_vox_file_data) {
    std::array<u8, 256> bin_map;
    std::fill(bin_map.begin(), bin_map.end(), 0);
    for (const auto& vox_chunk : parsed_vox_file_data.vox_chunks) {
        for (const auto xyzi : vox_chunk.XYZIs) {
            bin_map[xyzi[3]] = 1;
        }
    }

    std::vector<u8> result;

    u8 i{ 0U };
    for (const auto value : bin_map) {
        if (value) {
            result.push_back(i);
        }
        ++i;
    }

    return result;
}

ParsedVoxFileData parseVoxFile(const std::fs::path& in_vox_file_path) {
    std::ifstream stream(in_vox_file_path);

    char header[4] = {0};
    auto read_bytes = stream.readsome(header, sizeof(header));
    if (read_bytes != sizeof(header)) {
        throw std::runtime_error(fmt::format("file {} is less than 4 bytes couldn't find VOX header", in_vox_file_path.c_str()));
    }

    if (std::memcmp(header, "VOX ", sizeof(header)) != 0) {
        throw std::runtime_error(fmt::format("couldn't find VOX header in file {}", in_vox_file_path.c_str()));
    }

    u32 vox_format_version{ 0U };
    read_bytes = stream.readsome(reinterpret_cast<char*>(&vox_format_version), sizeof(vox_format_version));
    // const auto result = std::from_chars(header, header + 4, vox_format_version);
    if (read_bytes != sizeof(vox_format_version)) {//result.ec == std::errc::invalid_argument || result.ec == std::errc::result_out_of_range) {
        throw std::runtime_error(fmt::format("couldn't parse version from VOX header in file {}", in_vox_file_path.c_str()));
    }

    spdlog::info("Vox format version :: {}", vox_format_version);


    const auto main_chunk_header = parseVoxChunkHeader(stream);
    if (!main_chunk_header.has_value() || std::memcmp(main_chunk_header.value().id, "MAIN", 4) != 0) {
        throw std::runtime_error(fmt::format("couldn't parse main chunk header in VOX file {}", in_vox_file_path.c_str()));
    }
    printVoxChunkHeader(main_chunk_header.value());

    ParsedVoxFileData result; 
    for (u32 file_size{ 0U }; file_size < main_chunk_header.value().children_size;) {
        const auto vox_chunk_header = parseVoxChunkHeader(stream);
        if (!vox_chunk_header.has_value()) {
            throw std::runtime_error(fmt::format("couldn't parse chunk header in VOX file {}", in_vox_file_path.c_str()));
        }
        printVoxChunkHeader(vox_chunk_header.value());

        switch(*reinterpret_cast<const u32*>(vox_chunk_header->id)) {
        case CHUNK_ID_SIZE: { 
            auto& vox_chunk = result.vox_chunks.emplace_back();
            read_bytes = stream.readsome(reinterpret_cast<char*>(&vox_chunk.SIZE), sizeof(vox_chunk.SIZE));
            if (read_bytes != sizeof(vox_chunk.SIZE)) {
                throw std::runtime_error(fmt::format("failed to parse SIZE chunk in VOX file {}", in_vox_file_path.c_str()));
            }
            break; 
        }
        case CHUNK_ID_XYZI: { 
            auto& vox_chunk = result.vox_chunks.back();

            u32 voxels_in_chunk{ 0U };
            read_bytes = stream.readsome(reinterpret_cast<char*>(&voxels_in_chunk), sizeof(voxels_in_chunk));
            if (read_bytes != sizeof(voxels_in_chunk)) {
                throw std::runtime_error(fmt::format("failed to parse XYZI chunk in VOX file {}", in_vox_file_path.c_str()));
            }
            vox_chunk.XYZIs.resize(voxels_in_chunk, {0});
            stream.read(reinterpret_cast<char*>(vox_chunk.XYZIs.data()), voxels_in_chunk * sizeof(Vec4u8));
            if (!stream.good()) {
                throw std::runtime_error(fmt::format("failed to parse XYZI chunk in VOX file {}", in_vox_file_path.c_str()));
            }
            break; 
        }
        case CHUNK_ID_RGBA: {
            if (vox_chunk_header->content_size != sizeof(ParsedVoxFileData::palette)) {
                throw std::runtime_error(fmt::format("failed to parse RGBA chunk in VOX file {}", in_vox_file_path.c_str()));
            }
            stream.read(reinterpret_cast<char*>(result.palette.data()), vox_chunk_header->content_size);    
            if (!stream.good()) {
                throw std::runtime_error(fmt::format("failed to parse RGBA chunk in VOX file {}", in_vox_file_path.c_str()));
            }
            break; 
        }
        default: {
            printVoxChunkUnknownWarn(vox_chunk_header.value());
            stream.ignore(vox_chunk_header.value().content_size);
            break;
        }
        }

        file_size += vox_chunk_header.value().content_size + sizeof(VoxChunkHeader);
    }

    stream.close();

    for (auto& vox_chunk : result.vox_chunks) {
        std::sort(vox_chunk.XYZIs.begin(), vox_chunk.XYZIs.end(), 
        [](const Vec4u8& lhs, const Vec4u8& rhs) {
            return lhs[3] < rhs[3];
        });
    }

    return result;
}

// implicit
// struct VE0Chunk {
//     u32 bits_per_voxel;
//     std::vector<u32> state_dictionary;
//     std::vector<u64> compressed_data;
// };

struct VE0Region {
    u8 header[4]{ 'V', 'E', '0', ' ' };
    Vec3i32 region_resolution;
    Vec3i32 chunk_resolution;
    std::vector<u32> offsets;
    std::vector<std::vector<u64>> chunks;
};

struct VE0RegionWriter {
    i32 vox_chunk_index{ 0U };
    std::array<u8, 256> color_indices_translation_table{{0}};
    std::vector<u8> chunk;
    std::vector<std::vector<Vec4u8>> xyzi_per_chunk;
    Vec3i32 out_chunk_size;
    Vec3i32 ve0_chunks_in_vox_chunk; 
    i32 chunk_plane_size;
    i32 plane_size;
    i32 chunks_count;
    Vec3i32 out_region_size;
    VE0Region current_region;
    i32 base_region_file_offset;
    ZSTD_CCtx* zstd_compression_context;

    VE0RegionWriter(Vec3i32 out_chunk_size, Vec3i32 out_region_size) 
        : out_chunk_size(out_chunk_size), out_region_size(out_region_size), 
          chunk(out_chunk_size[0] * out_chunk_size[1] * out_chunk_size[2], 0U), 
          zstd_compression_context{ZSTD_createCCtx()} {
        current_region.chunk_resolution = out_chunk_size;
        current_region.region_resolution = out_region_size;
        current_region.offsets.resize(out_region_size[0]*out_region_size[1]*out_region_size[2], 0U);
        base_region_file_offset = 
            sizeof(VE0Region::header) +
            sizeof(VE0Region::region_resolution) +
            sizeof(VE0Region::chunk_resolution) +
            current_region.offsets.size() * sizeof(u32);
    }

    std::vector<u64> makeVE0Chunk(const std::array<u8, 256>& chunk_color_index_to_state, const std::vector<u32>& global_state_to_chunk_state) {
        const u32 states_count = static_cast<u32>(global_state_to_chunk_state.size());
        const auto bits_per_voxel = states_count == 1 ? 1 : static_cast<u32>(std::ceil(std::log2(static_cast<f32>(states_count))));
        const auto voxels_on_u64 = sizeof(u64) * 8UL / bits_per_voxel;
        const auto precompressed_chunk_size = 
            chunk.size() / voxels_on_u64 + 
            static_cast<std::size_t>(chunk.size() % voxels_on_u64 != 0); 

        std::vector<u64> ve0_chunk(
            (sizeof(bits_per_voxel) + sizeof(u32) * states_count)/2 +
            static_cast<std::size_t>((sizeof(bits_per_voxel) + sizeof(u32) * states_count)%2 != 0) +
            precompressed_chunk_size,
            0UL
        );

        std::size_t data_i{ 0U };
        // std::memcpy(static_cast<void*>(ve0_chunk.data()), static_cast<const void*>(&bits_per_voxel), sizeof(bits_per_voxel));
        // data_i += sizeof(bits_per_voxel);
        std::memcpy(static_cast<void*>(ve0_chunk.data()), static_cast<const void*>(&states_count), sizeof(states_count));
        data_i += sizeof(states_count);
        std::memcpy(
            reinterpret_cast<u8*>(ve0_chunk.data()) + data_i,
            static_cast<const void*>(global_state_to_chunk_state.data()),
            states_count * sizeof(u32)
        );
        data_i += states_count * sizeof(u32);

        std::size_t i{ 0U };
        std::vector<u64> precompressed_chunk(precompressed_chunk_size, 0U);
        u8 compressed_bits_counter{ 0U };
        for (const auto value : chunk) {
            precompressed_chunk[i] |= ((static_cast<u64>(chunk_color_index_to_state[value]) + 1) << compressed_bits_counter);
            if (compressed_bits_counter + bits_per_voxel > sizeof(u64)*8U) {
                compressed_bits_counter = 0U;
                ++i;
            } else {
                compressed_bits_counter += bits_per_voxel;
            }
        }

        const auto compressed_chunk_size = ZSTD_compressCCtx(
            zstd_compression_context,
            static_cast<void*>(reinterpret_cast<u8*>(ve0_chunk.data()) + data_i),
            precompressed_chunk_size * sizeof(u64),
            static_cast<const void*>(precompressed_chunk.data()),
            precompressed_chunk_size * sizeof(u64),
            22
        );

        std::vector<u64> result(
            (data_i + compressed_chunk_size)/sizeof(u64) + static_cast<std::size_t>((data_i + compressed_chunk_size) % sizeof(u64) != 0), 
            0
        );
        std::memcpy(static_cast<void*>(result.data()), static_cast<const void*>(ve0_chunk.data()), data_i + compressed_chunk_size);

        return result;
    }

    bool makeVE0Region(Vec3i32 p0, Vec3i32 p1) {
        std::fill(current_region.offsets.begin(), current_region.offsets.end(), 0U);
        current_region.chunks.clear();

        i32 region_file_offset{ base_region_file_offset };

        i32 in_region_i{ 0 };
        std::vector<u32> global_state_to_chunk_state;
        std::array<u8, 256> chunk_color_indices_translation_table{{0}};
        for (i32 z{ p0[2] }; z < p1[2]; ++z) {
            for (i32 y{ p0[1] }; y < p1[1]; ++y) {
                for (i32 x{ p0[0] }; x < p1[0]; ++x) {
                    const auto i = x + y * ve0_chunks_in_vox_chunk[0] + z * plane_size;
                    if (xyzi_per_chunk[i].empty()) {
                        ++in_region_i;
                        continue;
                    }
                    const Vec3i32 normalization_offset {
                        x * static_cast<i32>(out_chunk_size[0]),
                        y * static_cast<i32>(out_chunk_size[1]), 
                        z * static_cast<i32>(out_chunk_size[2])
                    };
                    u8 current_states_count{ 1U };
                    u8 previous_color_index{ xyzi_per_chunk[i][0][3] };
                    global_state_to_chunk_state.clear();
                    global_state_to_chunk_state.push_back(color_indices_translation_table[xyzi_per_chunk[i][0][3]]);
                    for (const auto xyzi : xyzi_per_chunk[i]) {
                        const Vec3i32 xyz {
                            static_cast<i32>(xyzi[0]), 
                            static_cast<i32>(xyzi[1]), 
                            static_cast<i32>(xyzi[2])
                        };
                        const auto normalized_xyzi = Vec3i32::sub(xyz, normalization_offset);
                        const auto index = 
                            normalized_xyzi[0] + 
                            normalized_xyzi[1] * out_chunk_size[0] +
                            normalized_xyzi[2] * chunk_plane_size;
                        chunk[index] = xyzi[3];
                        if (xyzi[3] != previous_color_index) {
                            chunk_color_indices_translation_table[xyzi[3]] = current_states_count++;
                            previous_color_index = xyzi[3];
                            global_state_to_chunk_state.push_back(color_indices_translation_table[xyzi[3]]);
                        }
                    }

                    const auto ve0_chunk = makeVE0Chunk(
                        chunk_color_indices_translation_table,
                        global_state_to_chunk_state
                    );
                    
                    current_region.offsets[in_region_i] = region_file_offset;
                    region_file_offset += ve0_chunk.size() * sizeof(u64);
                    current_region.chunks.emplace_back(std::move(ve0_chunk));

                    std::fill(chunk.begin(), chunk.end(), 0);
                    // std::fill(color_indices_translation_table.begin(), color_indices_translation_table.end(), 0);
                    ++in_region_i;
                }
            }
        }

        return !current_region.chunks.empty();
    }

    u32 next(const VoxChunk& vox_chunk, const std::fs::path& output_ve0_dir) {
        ve0_chunks_in_vox_chunk = Vec3i32{
            static_cast<i32>(vox_chunk.SIZE[0] / out_chunk_size[0] + static_cast<u32>(vox_chunk.SIZE[0] % out_chunk_size[0] != 0)),
            static_cast<i32>(vox_chunk.SIZE[1] / out_chunk_size[1] + static_cast<u32>(vox_chunk.SIZE[1] % out_chunk_size[1] != 0)),
            static_cast<i32>(vox_chunk.SIZE[2] / out_chunk_size[2] + static_cast<u32>(vox_chunk.SIZE[2] % out_chunk_size[2] != 0))
        };

        chunks_count = ve0_chunks_in_vox_chunk[0] * ve0_chunks_in_vox_chunk[1] * ve0_chunks_in_vox_chunk[2];
        spdlog::info("There are {}x{}x{}({}) ve0 chunks for {}x{}x{} chunk size", 
            ve0_chunks_in_vox_chunk[0], ve0_chunks_in_vox_chunk[1], ve0_chunks_in_vox_chunk[2], chunks_count,
            out_chunk_size[0], out_chunk_size[1], out_chunk_size[2]
        );

        plane_size = ve0_chunks_in_vox_chunk[0] * ve0_chunks_in_vox_chunk[1];
    
        for (auto& xyzi : xyzi_per_chunk) {
            xyzi.clear();
        }
        xyzi_per_chunk.resize(chunks_count);
        {
            u8 current_states_count{ 1U };
            u8 previous_color_index{ 0U };
            for (const auto xyzi : vox_chunk.XYZIs) {
                const auto index = 
                    xyzi[0] / out_chunk_size[0] +
                   (xyzi[1] / out_chunk_size[1]) * ve0_chunks_in_vox_chunk[0] +
                   (xyzi[2] / out_chunk_size[2]) * plane_size;
                xyzi_per_chunk[index].push_back(xyzi);
                if (xyzi[3] != previous_color_index) {
                    color_indices_translation_table[xyzi[3]] = current_states_count++;
                    previous_color_index = xyzi[3];
                }
            }
        }

        chunk_plane_size = static_cast<i32>(out_chunk_size[0] * out_chunk_size[1]);

        const Vec3i32 regions_count {
            (ve0_chunks_in_vox_chunk[0] / out_region_size[0]) + static_cast<i32>(ve0_chunks_in_vox_chunk[0] % out_region_size[0] != 0),
            (ve0_chunks_in_vox_chunk[1] / out_region_size[1]) + static_cast<i32>(ve0_chunks_in_vox_chunk[1] % out_region_size[1] != 0),
            (ve0_chunks_in_vox_chunk[2] / out_region_size[2]) + static_cast<i32>(ve0_chunks_in_vox_chunk[2] % out_region_size[2] != 0),
        };

        u32 overall_data_size{ 0 };

        for (i32 z{ 0 }; z < regions_count[2]; ++z) {
            for (i32 y{ 0 }; y < regions_count[1]; ++y) {
                for (i32 x{ 0 }; x < regions_count[0]; ++x) {
                    const auto p0 = Vec3i32::mul({x, y, z}, out_region_size);

                    auto p1 = Vec3i32::add(p0, out_region_size);
                    p1[0] = std::clamp(p1[0], p0[0], ve0_chunks_in_vox_chunk[0]);
                    p1[1] = std::clamp(p1[1], p0[1], ve0_chunks_in_vox_chunk[1]);
                    p1[2] = std::clamp(p1[2], p0[2], ve0_chunks_in_vox_chunk[2]);

                    if (makeVE0Region(p0, p1)) {
                        const auto path = output_ve0_dir / fmt::format("{}.{}.{}.{}.ve0", 
                            vox_chunk_index,
                            x - regions_count[0]/2, 
                            y - regions_count[1]/2,
                            z - regions_count[2]/2                                             
                        );

                        std::ofstream stream(path);
                        stream.write(reinterpret_cast<const char*>(&current_region), 
                            sizeof(VE0Region::header) +
                            sizeof(VE0Region::region_resolution) +
                            sizeof(VE0Region::chunk_resolution)
                        );
                        stream.write(reinterpret_cast<const char*>(current_region.offsets.data()), current_region.offsets.size() * sizeof(u32));
                        for (const auto& region_chunk : current_region.chunks) {
                            stream.write(reinterpret_cast<const char*>(region_chunk.data()), region_chunk.size() * sizeof(u64));
                        }
                    
                        overall_data_size += stream.tellp();

                        stream.close();
                    }
                }
            }
        }
        ++vox_chunk_index;

        return overall_data_size;
    }

    ~VE0RegionWriter() {
        ZSTD_freeCCtx(zstd_compression_context);
    }
};

#ifdef DEBUG
int main() {
    int argc = 17;
    const char* argv[] {
        "./build-rel-glfw3/src/tools/vox2ve0", "-i", "/home/regu/Downloads/monu9.vox", "-o", "/home/regu/codium_repos/VE-001/maps/test3/", 
        "-x", "16", "-y", "16", "-z", "16",
        "-X", "1", "-Y", "1", "-Z", "1",
    };
#else
int main(int argc, char **argv) {
#endif
    CLI::App app("Simple tool for converting from .vox file format to .ve0 file format", "vox2ve0");

    std::string input_vox_file;
    std::string output_ve0_dir;
    Vec3i32 out_chunk_size;
    Vec3i32 out_region_size;

    app.add_option("-i,--input", input_vox_file, "path to exisitng .vox file")
        ->required()
        ->check(CLI::ExistingFile);

    app.add_option("-o,--output", output_ve0_dir, "path to directory where to output .ve0 files")
        ->required()
        ->check(CLI::ExistingDirectory);

    app.add_option("-x,--chunk-size-x", out_chunk_size[0], "output chunk size in x axis")
        ->required();

    app.add_option("-y,--chunk-size-y", out_chunk_size[1], "output chunk size in y axis")
        ->required();

    app.add_option("-z,--chunk-size-z", out_chunk_size[2], "output chunk size in z axis")
        ->required();

    app.add_option("-X,--region-size-x", out_region_size[0], "output region size in x axis")
        ->required();

    app.add_option("-Y,--region-size-y", out_region_size[1], "output region size in y axis")
        ->required();

    app.add_option("-Z,--region-size-z", out_region_size[2], "output region size in z axis")
        ->required();

    app.callback([&]() {
        try {
            const auto parsed_vox_file_data = parseVoxFile(input_vox_file);
            const auto active_colors = fetchActiveColorIndicesFromPalette(parsed_vox_file_data);
            std::string dims_str = "";
            for (const auto& vox_chunk : parsed_vox_file_data.vox_chunks) {
                dims_str += fmt::format("<{}, {}, {}>, ", 
                    vox_chunk.SIZE[0],
                    vox_chunk.SIZE[1],
                    vox_chunk.SIZE[2]
                );
            }
            dims_str.pop_back();
            dims_str.pop_back();
            spdlog::info("Chunk dimensions are :: [ {} ]", dims_str);
            spdlog::info("Number of unique color indices :: {}", active_colors.size());
            std::string colors_str = "";
            for (const auto color_index : active_colors) {
                const auto color = parsed_vox_file_data.palette[color_index+1];
                colors_str += fmt::format(fmt::fg(fmt::rgb(color[2], color[1], color[0])), "{}:{:8x}, ", color_index, *reinterpret_cast<const u32*>(&color));
            }
            colors_str.pop_back();
            colors_str.pop_back();
            spdlog::info("Colors are :: [ {} ]", colors_str);

            VE0RegionWriter writer(out_chunk_size, out_region_size);

            u32 overall_data_size{ 0U };
            for (const auto& vox_chunk : parsed_vox_file_data.vox_chunks) {
                overall_data_size += writer.next(vox_chunk, output_ve0_dir);
            }
            spdlog::info("written .ve0 files of overall size {}B", overall_data_size);

        } catch (const std::exception& e) {
            spdlog::error("{}", e.what());
        }
    });

    CLI11_PARSE(app, argc, argv);
}