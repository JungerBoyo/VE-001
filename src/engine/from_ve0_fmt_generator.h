#ifndef VE001_FROM_VE0_FMT_GENERATOR_H
#define VE001_FROM_VE0_FMT_GENERATOR_H

#include <filesystem>

#include "chunk_generator.h"

namespace ve001 {

struct FromVE0FmtGenerator : public ChunkGenerator {
    FromVE0FmtGenerator(const std::filesystem::path& ve0_dir_path);

    void threadInit() override;
    std::optional<std::span<const vmath::u16>> gen(vmath::Vec3i32 chunk_position) override;
};

}

#endif