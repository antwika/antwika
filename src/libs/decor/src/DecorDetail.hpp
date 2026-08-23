#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <vector>

#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include <antwika/decor/Decor.hpp>

namespace antwika::decor::decordetail
{

    inline constexpr std::uint64_t kMaxSolveSteps = 20000;

    struct SeamTables final
    {
        wfc::CompatibilityTable horizontalInteriorTable;

        wfc::CompatibilityTable horizontalBoundaryTable;

        wfc::CompatibilityTable verticalInteriorTable;

        wfc::CompatibilityTable verticalBoundaryTable;
    };

    [[nodiscard]] bool isSeamCompatible(
        const tile::TileRules &rules,
        tilemap::Tile oneTile,
        voxel::Side side,
        voxel::EdgeKind kind,
        tilemap::Tile otherTile);

    [[nodiscard]] std::vector<std::size_t> getShuffledValues(
        std::size_t many, std::uint32_t seed);

    [[nodiscard]] std::uint32_t getHashMix(std::uint32_t value);

    [[nodiscard]] std::uint8_t frequencyRollFor(
        voxel::VoxelPosition position,
        std::size_t which,
        std::uint32_t seed,
        std::uint32_t stir);

    [[nodiscard]] voxel::VoxelPosition getWallTangent(std::size_t side);


    [[nodiscard]] std::map<std::size_t, tilemap::Tile> getPlaceSpannedDecor(
        const std::vector<voxelmap::FaceRef> &faces,
        std::span<const tilemap::Tile> drawnTiles,
        std::span<const DecorTile> decor,
        std::uint32_t seed);

    template <typename Meets>
    [[nodiscard]] SeamTables seamTables(
        const std::size_t size, const Meets &meets)
    {
        SeamTables seamTables{
            wfc::CompatibilityTable(size),
            wfc::CompatibilityTable(size),
            wfc::CompatibilityTable(size),
            wfc::CompatibilityTable(size)};

        for (std::size_t one = 0; one < size; ++one)
        {
            for (std::size_t other = 0; other < size; ++other)
            {
                seamTables.horizontalInteriorTable.set(
                    one,
                    other,
                    meets(
                        one,
                        voxel::Side::Right,
                        voxel::EdgeKind::Interior,
                        other));
                seamTables.horizontalBoundaryTable.set(
                    one,
                    other,
                    meets(
                        one,
                        voxel::Side::Right,
                        voxel::EdgeKind::Boundary,
                        other));
                seamTables.verticalInteriorTable.set(
                    one,
                    other,
                    meets(
                        one,
                        voxel::Side::Bottom,
                        voxel::EdgeKind::Interior,
                        other));
                seamTables.verticalBoundaryTable.set(
                    one,
                    other,
                    meets(
                        one,
                        voxel::Side::Bottom,
                        voxel::EdgeKind::Boundary,
                        other));
            }
        }

        return seamTables;
    } // GCOVR_EXCL_LINE

}
