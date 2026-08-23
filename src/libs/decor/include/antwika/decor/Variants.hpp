#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include <antwika/widget/WidgetId.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/tile/TileRules.hpp>

#include <antwika/decor/Decor.hpp>

#include "antwika/decor/VariantGroup.hpp"
#include "antwika/decor/VariantMember.hpp"

namespace antwika::decor
{

    [[nodiscard]] const VariantGroup *getGroupLedBy(
        std::span<const VariantGroup> familyGroups, tilemap::Tile tile);

    [[nodiscard]] const VariantGroup *getGroupContaining(
        std::span<const VariantGroup> familyGroups, tilemap::Tile tile);

    [[nodiscard]] tilemap::Tile canonicalTileOf(
        std::span<const VariantGroup> familyGroups, tilemap::Tile tile);

    [[nodiscard]] std::vector<VariantGroup> getWithVariantToggled(
        const std::vector<VariantGroup> &familyGroups,
        tilemap::Tile canonicalTile,
        tilemap::Tile tile);

    [[nodiscard]] std::vector<VariantGroup> getWithVariantWeightSet(
        const std::vector<VariantGroup> &familyGroups,
        tilemap::Tile tile,
        std::uint8_t weight);

    [[nodiscard]] bool canBeVariantOf(
        std::span<const VariantGroup> familyGroups,
        const tile::TileRules &rules,
        std::span<const DecorTile> decor,
        tilemap::Tile canonicalTile,
        tilemap::Tile tile);

    [[nodiscard]] std::vector<tilemap::Tile> getWithVariantsApplied(
        const std::vector<voxelmap::FaceRef> &faces,
        std::span<const tilemap::Tile> wovenTiles,
        std::span<const VariantGroup> familyGroups,
        std::uint32_t seed);

    inline constexpr widget::WidgetId kVariantChoiceWidget{348};

    inline constexpr widget::WidgetId kVariantWeightWidget{349};

    inline constexpr widget::WidgetId kGoToCanonicalWidget{350};

}
