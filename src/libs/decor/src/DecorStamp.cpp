#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <utility>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include <antwika/decor/Decor.hpp>
#include "DecorDetail.hpp"

namespace antwika::decor::decordetail
{

    namespace
    {
        [[nodiscard]] voxel::VoxelCell spanDown(const std::size_t side)
        {
            return side == 4 ? voxel::VoxelCell{.z = 1}
                         : voxel::VoxelCell{.y = -1};
        }

        [[nodiscard]] voxel::VoxelCell spanRight(const std::size_t side)
        {
            return side == 4 ? voxel::VoxelCell{.x = 1}
                         : wallTangent(side);
        }

        [[nodiscard]] bool takes(
            const DecorTile &decor, const tilemap::Tile baseTile)
        {
            return decor.tile.atlas == baseTile.atlas
                   && std::find(
                          decor.allowedBaseTiles.begin(),
                          decor.allowedBaseTiles.end(),
                          baseTile)
                          != decor.allowedBaseTiles.end();
        }
    }

    std::map<std::size_t, tilemap::Tile> placeSpannedDecor(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::span<const tilemap::Tile> drawnTiles,
        const std::span<const DecorTile> decor,
        const std::uint32_t seed)
    {
        std::map<std::size_t, tilemap::Tile> tile;
        std::map<std::pair<std::size_t, voxel::VoxelCell>, std::size_t>
            index;

        for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex)
        {
            if (gfx::Vec3(voxelmap::faceNormal(faces[faceIndex].side)).y
                < 0.0F)
            {
                continue;
            }

            index.emplace(
                std::pair{
                    faces[faceIndex].side, positionOnly(faces[faceIndex].cell)},
                faceIndex);
        }

        for (const auto which :
             shuffledValues(decor.size(), seed))
        {
            const auto &record = decor[which];

            if (!decorSpanned(record))
            {
                continue;
            }

            for (std::size_t faceIndex = 0; faceIndex < faces.size();
                 ++faceIndex)
            {
                const auto side = faces[faceIndex].side;
                const auto upward = side == 4;

                if (!index.contains(
                        std::pair{
                            side, positionOnly(faces[faceIndex].cell)})
                    || tile.contains(faceIndex)
                    || !takes(record, drawnTiles[faceIndex])
                    || frequencyRollFor(
                           faces[faceIndex].cell,
                           which,
                           seed,
                           upward
                               ? 0U
                               : static_cast<std::uint32_t>(
                                     side + 1))
                           >= record.frequency)
                {
                    continue;
                }

                const auto right = spanRight(side);
                const auto downSpan = spanDown(side);
                std::vector<std::size_t> coveredIndexes;
                auto allCovered = true;

                for (std::uint8_t row = 0;
                     allCovered && row < record.height;
                     ++row)
                {
                    for (std::uint8_t column = 0;
                         allCovered && column < record.width;
                         ++column)
                    {
                        const auto cell = positionOnly(
                            faces[faceIndex].cell);
                        const auto overCell = voxel::VoxelCell{
                            .x = cell.x + (right.x * column)
                                 + (downSpan.x * row),
                            .y = cell.y + (right.y * column)
                                 + (downSpan.y * row),
                            .z = cell.z + (right.z * column)
                                 + (downSpan.z * row)};
                        const auto foundEntry = index.find(
                            std::pair{side, overCell});

                        if (foundEntry == index.end()
                            || tile.contains(foundEntry->second)
                            || !takes(
                                record, drawnTiles[foundEntry->second]))
                        {
                            allCovered = false;

                            continue;
                        }

                        coveredIndexes.push_back(foundEntry->second);
                    }
                }

                if (!allCovered)
                {
                    continue;
                }

                for (std::size_t place = 0;
                     place < coveredIndexes.size();
                     ++place)
                {
                    tile.emplace(
                        coveredIndexes[place],
                        record.spanTiles[place]);
                }
            }
        }

        return tile;
    } // GCOVR_EXCL_LINE

}
