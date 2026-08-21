#pragma once

#include <cstdint>
#include <vector>
#include <antwika/decor/Decor.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include "antwika/decor/VariantMember.hpp"

namespace antwika::decor
{

    struct VariantGroup final
    {
        tilemap::Tile canonicalTile{};

        std::uint8_t weight = kFullFrequency;

        std::vector<VariantMember> variants{};

        [[nodiscard]] bool operator==(
            const VariantGroup &other) const
            = default;
    };

}
