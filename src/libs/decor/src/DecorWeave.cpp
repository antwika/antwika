#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <utility>

#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/SolverLimits.hpp>

#include <antwika/decor/Decor.hpp>

#include "DecorDetail.hpp"

namespace antwika::decor
{

    using namespace decordetail;

    namespace decordetail
    {
        std::uint32_t hashMix(std::uint32_t value)
        {
            value ^= value << 13U;
            value ^= value >> 17U;
            value ^= value << 5U;

            return value;
        }

        std::uint8_t frequencyRollFor(
            const voxel::VoxelPosition position,
            const std::size_t which,
            const std::uint32_t seed,
            const std::uint32_t stir)
        {
            auto mixedSeed = static_cast<std::uint32_t>(position.x)
                         * 73856093U;

            mixedSeed ^= static_cast<std::uint32_t>(position.y)
                     * 19349663U;
            mixedSeed ^= static_cast<std::uint32_t>(position.z)
                     * 83492791U;
            mixedSeed ^= static_cast<std::uint32_t>(which)
                     * 2654435761U;
            mixedSeed ^= seed * 2246822519U;
            mixedSeed ^= stir * 0x27d4eb2fU;

            return static_cast<std::uint8_t>(
                hashMix(mixedSeed | 1U) % kFullFrequency);
        }

        voxel::VoxelPosition wallTangent(const std::size_t side)
        {
            switch (side)
            {
            case 0:
                return voxel::VoxelPosition{.x = 1};
            case 1:
                return voxel::VoxelPosition{.x = -1};
            case 2:
                return voxel::VoxelPosition{.z = -1};
            default:
                return voxel::VoxelPosition{.z = 1};
            }
        }

        std::vector<std::size_t> shuffledValues(
            const std::size_t many, std::uint32_t seed)
        {
            std::vector<std::size_t> orderIndexes(many);

            for (std::size_t index = 0; index < many; ++index)
            {
                orderIndexes[index] = index;
            }

            seed = hashMix(seed | 1U);

            for (std::size_t index = many; index > 1; --index)
            {
                seed = hashMix(seed);
                std::swap(orderIndexes[index - 1], orderIndexes[seed % index]);
            }

            return orderIndexes;
        } // GCOVR_EXCL_LINE

        std::uint32_t choiceRollFor(
            const voxel::VoxelPosition position,
            const std::size_t side,
            const std::uint32_t seed)
        {
            auto mixedSeed = static_cast<std::uint32_t>(position.x)
                         * 73856093U;

            mixedSeed ^= static_cast<std::uint32_t>(position.y)
                     * 19349663U;
            mixedSeed ^= static_cast<std::uint32_t>(position.z)
                     * 83492791U;
            mixedSeed ^= static_cast<std::uint32_t>(side + 1)
                     * 0x9e3779b9U;
            mixedSeed ^= seed * 2246822519U;

            return hashMix(mixedSeed | 1U);
        }

        bool seamCompatible(
            const tile::TileRules &rules,
            const tilemap::Tile oneTile,
            const voxel::Side side,
            const voxel::EdgeKind kind,
            const tilemap::Tile otherTile)
        {
            const tilemap::TileEdge nearEdge{.side = side, .edge = kind};
            const auto farFacing = voxel::facing(nearEdge);

            return tilesCompatible(rules, oneTile, nearEdge, otherTile)
                   && tilesCompatible(rules, otherTile, farFacing, oneTile);
        }
    }

    std::vector<std::pair<std::size_t, std::map<std::size_t, tilemap::Tile>>>
    solveDecorLayers(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::span<const tilemap::Tile> drawnTiles,
        const std::span<const DecorTile> decor,
        const tile::TileRules &decorRules,
        const std::uint32_t seed)
    {
        std::set<std::size_t> layers;

        for (const auto &record : decor)
        {
            layers.insert(record.layer);
        }

        std::vector<
            std::pair<std::size_t, std::map<std::size_t, tilemap::Tile>>>
            layerGroups;

        for (const auto layer : layers)
        {
            std::vector<DecorTile> ownTiles;

            for (const auto &record : decor)
            {
                if (record.layer == layer)
                {
                    ownTiles.push_back(record);
                }
            }

            auto placedDecor = solveDecor(
                faces,
                drawnTiles,
                ownTiles,
                decorRules,
                seed
                    ^ hashMix(
                        static_cast<std::uint32_t>(layer) | 1U));

            if (!placedDecor.empty())
            {
                layerGroups.emplace_back(layer, std::move(placedDecor));
            }
        }

        return layerGroups;
    } // GCOVR_EXCL_LINE

    std::map<std::size_t, tilemap::Tile> solveDecor(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::span<const tilemap::Tile> drawnTiles,
        const std::span<const DecorTile> decor,
        const tile::TileRules &decorRules,
        const std::uint32_t seed)
    {
        if (decor.empty())
        {
            return {};
        }

        const auto stampedTiles =
            placeSpannedDecor(faces, drawnTiles, decor, seed);
        const auto blank = decor.size();
        const auto order = shuffledValues(decor.size(), seed);

        struct DecorPlacement final
        {
            std::size_t face = 0;
            voxel::VoxelPosition position{};
            std::size_t side = 0;
        };

        std::vector<DecorPlacement> decorPlacements;
        std::map<std::pair<std::int32_t, std::int32_t>, std::size_t>
            byGround;
        std::map<std::pair<std::size_t, voxel::VoxelPosition>, std::size_t>
            byWall;
        std::vector<wfc::Domain> waveDomains;
        std::vector<std::optional<std::size_t>> preferences;

        for (std::size_t index = 0; index < faces.size(); ++index)
        {
            const auto looking =
                gfx::Vec3(voxelmap::faceNormal(faces[index].side)).y;
            const auto upward = looking > 0.0F;

            if (looking < 0.0F || stampedTiles.contains(index))
            {
                continue;
            }

            wfc::Domain mayDomain(decor.size() + 1);
            std::vector<std::pair<std::size_t, std::uint32_t>>
                offeredTiles;

            for (std::size_t which = 0; which < decor.size(); ++which)
            {
                const auto &record = decor[which];
                const auto fits =
                    std::find(
                        record.allowedBaseTiles.begin(),
                        record.allowedBaseTiles.end(),
                        drawnTiles[index])
                    != record.allowedBaseTiles.end();

                if (fits && !decorSpanned(record)
                    && record.tile.atlas == drawnTiles[index].atlas
                    && frequencyRollFor(
                           faces[index].cell.position(),
                           which,
                           seed,
                           upward ? 0U
                                  : static_cast<std::uint32_t>(
                                        faces[index].side + 1))
                           < record.frequency)
                {
                    offeredTiles.emplace_back(which, record.weight);
                }
                else
                {
                    mayDomain.remove(order[which]);
                }
            }

            if (offeredTiles.empty())
            {
                continue;
            }

            auto total = std::uint32_t{0};

            for (const auto &[which, weight] : offeredTiles)
            {
                total += weight;
            }

            const auto evenly = total == 0;

            if (evenly)
            {
                total = static_cast<std::uint32_t>(
                    offeredTiles.size());
            }

            auto rollValue = choiceRollFor(
                              faces[index].cell.position(
                                  ), faces[index].side, seed)
                          % total;
            auto likedTile = offeredTiles.front().first;

            for (const auto &[which, weight] : offeredTiles)
            {
                const auto share =
                    evenly ? std::uint32_t{1} : weight;

                if (rollValue < share)
                {
                    likedTile = which;
                    break;
                }

                rollValue -= share;
            }

            preferences.push_back(order[likedTile]);

            if (upward)
            {
                byGround.emplace(
                    std::pair{
                        faces[index].cell.position().x,
                        faces[index].cell.position().z},
                    decorPlacements.size());
            }
            else
            {
                byWall.emplace(
                    std::pair{
                        faces[index].side,
                        faces[index].cell.position()},
                    decorPlacements.size());
            }

            decorPlacements.push_back(
                DecorPlacement{
                    .face = index,
                    .position = faces[index].cell.position(),
                    .side = faces[index].side});
            waveDomains.push_back(mayDomain);
        }

        if (decorPlacements.empty())
        {
            return stampedTiles;
        }

        const auto meets = [&decor, &decorRules, blank, &order](
                               const std::size_t one,
                               const voxel::Side side,
                               const voxel::EdgeKind kind,
                               const std::size_t other)
        {
            if (one == blank || other == blank)
            {
                return true;
            }

            std::size_t oneAt = blank;
            std::size_t otherAt = blank;

            for (std::size_t index = 0; index < order.size(); ++index)
            {
                if (order[index] == one)
                {
                    oneAt = index;
                }

                if (order[index] == other)
                {
                    otherAt = index;
                }
            }

            return seamCompatible(
                             decorRules,
                             decor[oneAt].tile,
                             side,
                             kind,
                             decor[otherAt].tile);
        };

        const auto tables =
            seamTables(decor.size() + 1, meets);

        std::vector<wfc::AdjacencyConstraint> adjacencies;

        for (std::size_t index = 0; index < decorPlacements.size(); ++index)
        {
            const auto &mine = decorPlacements[index].position;

            if (decorPlacements[index].side != 4)
            {
                const auto way = wallTangent(decorPlacements[index].side);
                const auto sideways = voxel::VoxelPosition{
                    .x = mine.x + way.x,
                    .y = mine.y,
                    .z = mine.z + way.z};
                const auto underPosition = voxel::VoxelPosition{
                    .x = mine.x, .y = mine.y - 1, .z = mine.z};
                const auto acrossWall = byWall.find(
                    std::pair{decorPlacements[index].side, sideways});
                const auto belowWall = byWall.find(
                    std::pair{decorPlacements[index].side, underPosition});

                if (acrossWall != byWall.end())
                {
                    const auto sameCube =
                        way.x != 0
                               ? voxel::cubeCornerOf(mine).x
                                  == voxel::cubeCornerOf(sideways).x
                                   : voxel::cubeCornerOf(mine).z
                                  == voxel::cubeCornerOf(sideways).z;

                    adjacencies.emplace_back(
                        index,
                        acrossWall->second,
                        sameCube ? tables.horizontalInteriorTable
                               : tables.horizontalBoundaryTable);
                }

                if (belowWall != byWall.end())
                {
                    const auto cubeOffset =
                        voxel::cubeCornerOf(mine).y
                        == voxel::cubeCornerOf(underPosition).y;

                    adjacencies.emplace_back(
                        index,
                        belowWall->second,
                        cubeOffset ? tables.verticalInteriorTable
                               : tables.verticalBoundaryTable);
                }

                continue;
            }

            const auto east = byGround.find(
                std::pair{mine.x + 1, mine.z});
            const auto south = byGround.find(
                std::pair{mine.x, mine.z + 1});

            if (east != byGround.end()
                && decorPlacements[east->second].position.y == mine.y)
            {
                const auto cubeOffset =
                    voxel::cubeCornerOf(mine).x
                    == voxel::cubeCornerOf(
                        decorPlacements[east->second].position).x;

                adjacencies.emplace_back(
                    index,
                    east->second,
                    cubeOffset ? tables.horizontalInteriorTable
                           : tables.horizontalBoundaryTable);
            }

            if (south != byGround.end()
                && decorPlacements[south->second].position.y == mine.y)
            {
                const auto cubeOffset =
                    voxel::cubeCornerOf(mine).z
                    == voxel::cubeCornerOf(
                        decorPlacements[south->second].position).z;

                adjacencies.emplace_back(
                    index,
                    south->second,
                    cubeOffset ? tables.verticalInteriorTable
                           : tables.verticalBoundaryTable);
            }
        }

        std::vector<std::reference_wrapper<const wfc::IConstraint>>
            constraints(adjacencies.begin(), adjacencies.end());

        const auto solution =
            wfc::Solver(
                std::move(waveDomains),
                std::move(constraints),
                {},
                wfc::SolverLimits{.maxSteps = kMaxSolveSteps},
                std::move(preferences))
                .solve();

        auto updatedStamps = stampedTiles;

        if (solution.outcome != wfc::SolveOutcome::Solved)
        {
            return updatedStamps;
        }

        for (std::size_t index = 0; index < decorPlacements.size(); ++index)
        {
            const auto value = solution.assignment[index];

            std::size_t which = blank;

            for (std::size_t place = 0; place < order.size();
                 ++place)
            {
                if (order[place] == value)
                {
                    which = place;
                }
            }

            if (which != blank)
            {
                updatedStamps.emplace(decorPlacements[index].face,
                decor[which].tile);
            }
        }

        return updatedStamps;
    } // GCOVR_EXCL_LINE

}
