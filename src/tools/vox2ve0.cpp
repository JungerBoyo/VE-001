#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

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
    std::array<u32, 256> palette;
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

            vox_chunk.XYZIs.resize(vox_chunk_header->content_size/sizeof(Vec4u8), {0});
            stream.read(reinterpret_cast<char*>(vox_chunk.XYZIs.data()), vox_chunk_header->content_size);
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
            [] (const Vec4u8& lhs, const Vec4u8& rhs) {
                if (lhs[2] != rhs[2]) {
                    return lhs[2] < rhs[2];
                } else if (lhs[1] != rhs[1]) {
                    return lhs[1] < rhs[1];
                } else {
                    return lhs[0] < rhs[0];
                };
            }
        );
    }

    return result;
}

void writeAsVE0Chunks(const ParsedVoxFileData& parsed_vox_file_data, Vec3u32 out_chunk_size, const std::fs::path& output_ve0_dir) {
    i32 vox_chunk_index{ 0U };
    for (const auto& vox_chunk : parsed_vox_file_data.vox_chunks) {
        const Vec3i32 ve0_chunks_in_vox_chunk {
            static_cast<i32>(vox_chunk.SIZE[0] / out_chunk_size[0] + static_cast<u32>(vox_chunk.SIZE[0] % out_chunk_size[0] != 0)),
            static_cast<i32>(vox_chunk.SIZE[1] / out_chunk_size[1] + static_cast<u32>(vox_chunk.SIZE[1] % out_chunk_size[1] != 0)),
            static_cast<i32>(vox_chunk.SIZE[2] / out_chunk_size[2] + static_cast<u32>(vox_chunk.SIZE[2] % out_chunk_size[2] != 0))
        };

        const auto chunks_count = Vec3i32::dot(ve0_chunks_in_vox_chunk, ve0_chunks_in_vox_chunk);
        spdlog::info("There are {}x{}x{}({}) ve0 chunks for {}x{}x{} chunk size", 
            ve0_chunks_in_vox_chunk[0], ve0_chunks_in_vox_chunk[1], ve0_chunks_in_vox_chunk[2], chunks_count,
            out_chunk_size[0], out_chunk_size[1], out_chunk_size[2]
        );

        const auto plane_size = ve0_chunks_in_vox_chunk[0] * ve0_chunks_in_vox_chunk[1];

        std::vector<std::vector<Vec4u8>> xyzi_per_chunk(chunks_count);
        for (const auto xyzi : vox_chunk.XYZIs) {
            const auto index = 
                xyzi[0] / ve0_chunks_in_vox_chunk[0] +
               (xyzi[1] / ve0_chunks_in_vox_chunk[1]) * ve0_chunks_in_vox_chunk[0] +
               (xyzi[2] / ve0_chunks_in_vox_chunk[2]) * plane_size;
            xyzi_per_chunk[index].push_back(xyzi);
        }

        std::vector<u8> chunk(out_chunk_size[0] * out_chunk_size[1] * out_chunk_size[2], 0U);

        const auto chunk_plane_size = static_cast<i32>(out_chunk_size[0] * out_chunk_size[1]);

        u32 i{ 0U };
        for (i32 z{ 0 }; z < ve0_chunks_in_vox_chunk[2]; ++z) {
            for (i32 y{ 0 }; y < ve0_chunks_in_vox_chunk[1]; ++y) {
                for (i32 x{ 0 }; x < ve0_chunks_in_vox_chunk[0]; ++x) {
                    const Vec3i32 normalization_offset {
                        x * static_cast<i32>(out_chunk_size[0]),
                        y * static_cast<i32>(out_chunk_size[1]), 
                        z * static_cast<i32>(out_chunk_size[2])
                    };
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
                    }

                    const auto path = output_ve0_dir / fmt::format("{}.{}.{}.{}.ve0", 
                        vox_chunk_index,
                        x - ve0_chunks_in_vox_chunk[0]/2, 
                        y - ve0_chunks_in_vox_chunk[1]/2, 
                        z - ve0_chunks_in_vox_chunk[2]/2                                             
                    );

                    std::ofstream ostream(path);

                    

                    ostream.close();

                    ++i;
                }
            }
        }
        ++vox_chunk_index;
    }
}

int main(int argc, char **argv) {
    CLI::App app("Simple tool for converting from .vox file format to .ve0 file format", "vox2ve0");

    std::string input_vox_file;
    std::string output_ve0_dir;
    Vec3u32 out_chunk_size;

    app.add_option("-i,--input", input_vox_file, "path to exisitng .vox file")
        ->required()
        ->check(CLI::ExistingFile);

    app.add_option("-o,--output", output_ve0_dir, "path to directory where to output .ve0 files")
        ->required()
        ->check(CLI::ExistingDirectory);

    app.add_option("-x,--chunk-size-x", out_chunk_size.values[0], "output chunk size in x axis")
        ->required();

    app.add_option("-y,--chunk-size-y", out_chunk_size.values[1], "output chunk size in y axis")
        ->required();

    app.add_option("-z,--chunk-size-z", out_chunk_size.values[2], "output chunk size in z axis")
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
               for (const auto pos : vox_chunk.XYZIs) {
                    fmt::print("[ {}, {}, {}, {} ]\n", pos[0], pos[1], pos[2], pos[3]);
                }
            }
            dims_str.pop_back();
            dims_str.pop_back();
            spdlog::info("Chunk dimensions are :: [ {} ]", dims_str);
            spdlog::info("Number of unique color indices :: {}", active_colors.size());
            std::string colors_str = "";
            for (const auto color_index : active_colors) {
                colors_str += fmt::format("{}:{:8x}, ", color_index, parsed_vox_file_data.palette[color_index]);
            }
            colors_str.pop_back();
            colors_str.pop_back();
            spdlog::info("Colors are :: [ {} ]", colors_str);

            writeAsVE0Chunks(parsed_vox_file_data, out_chunk_size, output_ve0_dir);
        } catch (const std::exception& e) {
            spdlog::error("{}", e.what());
        }
    });

    CLI11_PARSE(app, argc, argv);
}