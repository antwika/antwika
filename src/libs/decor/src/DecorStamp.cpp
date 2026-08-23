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
        [[nodiscard]] voxel::VoxelPosition getSpanDown(const std::size_t side)
        {
            return side == 4 ? voxel::VoxelPosition{.z = 1}
                         : voxel::VoxelPosition{.y = -1};
        }

        [[nodiscard]] voxel::VoxelPosition getSpanRight(const std::size_t side)
        {
            return side == 4 ? voxel::VoxelPosition{.x = 1}
                         : getWallTangent(side);
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

    std::map<std::size_t, tilemap::Tile> getPlaceSpannedDecor(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::span<const tilemap::Tile> drawnTiles,
        const std::span<const DecorTile> decor,
        const std::uint32_t seed)
    {
        std::map<std::size_t, tilemap::Tile> tile;
        std::map<std::pair<std::size_t, voxel::VoxelPosition>, std::size_t>
            index;

        for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex)
        {
            if (gfx::Vec3(voxelmap::getFaceNormal(faces[faceIndex].side)).y
                < 0.0F)
            {
                continue;
            }

            index.emplace(
                std::pair{
                    faces[faceIndex].side, faces[faceIndex].cell.position},
                faceIndex);
        }

        for (const auto which :
             getShuffledValues(decor.size(), seed))
        {
            const auto &record = decor[which];

            if (!isDecorSpanned(record))
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
                            side, faces[faceIndex].cell.position})
                    || tile.contains(faceIndex)
                    || !takes(record, drawnTiles[faceIndex])
                    || frequencyRollFor(
                           faces[faceIndex].cell.position,
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

                const auto right = getSpanRight(side);
                const auto downSpan = getSpanDown(side);
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
                        const auto position =
                            faces[faceIndex].cell.position;
                        const auto overPosition = voxel::VoxelPosition{
                            .x = position.x + (right.x * column)
                                 + (downSpan.x * row),
                            .y = position.y + (right.y * column)
                                 + (downSpan.y * row),
                            .z = position.z + (right.z * column)
                                 + (downSpan.z * row)};
                        const auto foundEntry = index.find(
                            std::pair{side, overPosition});

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
