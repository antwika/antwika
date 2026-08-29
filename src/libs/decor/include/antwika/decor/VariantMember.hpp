#pragma once

#include <cstdint>

#include <antwika/decor/Decor.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::decor
{

    struct VariantMember final
    {
        tilemap::Tile tile{};

        std::uint8_t weight = kFullFrequency;

        [[nodiscard]] bool operator==(
            const VariantMember &other) const
            = default;
    };

}
