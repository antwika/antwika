#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>

#include "antwika/worldgen/ChunkShape.hpp"

namespace antwika::worldgen
{

    struct District final
    {
        std::string name{};

        std::uint8_t untilShare = 100;

        std::vector<std::uint32_t> desire{};

        [[nodiscard]] bool operator==(const District &other) const
            = default;
    };

}
