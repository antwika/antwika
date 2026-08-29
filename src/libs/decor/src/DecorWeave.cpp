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
#include <antwika/voxel/FacingTraits.hpp>

#include "DecorDetail.hpp"

namespace antwika::decor
{

    using namespace decordetail;

    namespace decordetail
    {
        std::uint32_t getHashMix(std::uint32_t value)
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
                getHashMix(mixedSeed | 1U) % kFullFrequency);
        }

        voxel::VoxelPosition getWallTangent(const std::size_t side)
        {
            return voxel::stepOf(
                voxel::kCardinalFacings.at(
                    side % voxel::kCardinalFacings.size()));
        }

        std::vector<std::size_t> getShuffledValues(
            const std::size_t many, std::uint32_t seed)
        {
            std::vector<std::size_t> orderIndexes(many);

            for (std::size_t index = 0; index < many; ++index)
            {
                orderIndexes[index] = index;
            }

            seed = getHashMix(seed | 1U);

            for (std::size_t index = many; index > 1; --index)
            {
                seed = getHashMix(seed);
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

            return getHashMix(mixedSeed | 1U);
        }

        bool isSeamCompatible(
            const tile::TileRules &rules,
            const tilemap::Tile oneTile,
            const voxel::Side side,
            const voxel::EdgeKind kind,
            const tilemap::Tile otherTile)
        {
            const tilemap::TileEdge nearEdge{.side = side, .edge = kind};
            const auto farFacing = voxel::getFacing(nearEdge);

            return tilesCompatible(rules, oneTile, nearEdge, otherTile)
                   && tilesCompatible(rules, otherTile, farFacing, oneTile);
        }
    }

    namespace
    {
        struct DecorPlacement final
        {
            std::size_t face = 0;

            voxel::VoxelPosition position{};

            std::size_t side = 0;
        };

        struct DecorWeaveState final
        {
            const std::vector<voxelmap::FaceRef> &faces;
            const std::span<const tilemap::Tile> drawnTiles;
            const std::span<const DecorTile> decor;
            const tile::TileRules &decorRules;
            std::uint32_t seed;
            std::size_t blank;

            std::map<std::size_t, tilemap::Tile> stampedTiles{};
            std::vector<std::size_t> order{};
            std::vector<std::size_t> placeOf{};
            std::vector<DecorPlacement> decorPlacements{};
            std::map<std::pair<std::int32_t, std::int32_t>, std::size_t>
                byGround{};
            std::map<std::pair<std::size_t, voxel::VoxelPosition>,
                std::size_t>
                byWall{};
            std::vector<wfc::Domain> waveDomains{};
            std::vector<std::optional<std::size_t>> preferences{};
            std::vector<wfc::AdjacencyConstraint> adjacencies{};
        };

        void layShuffledOrder(DecorWeaveState &weave)
        {
            weave.order =
                getShuffledValues(weave.decor.size(), weave.seed);
            weave.placeOf.assign(weave.decor.size(), weave.blank);

            for (std::size_t place = 0; place < weave.order.size(); ++place)
            {
                weave.placeOf[weave.order[place]] = place;
            }
        }

        [[nodiscard]] std::size_t likedTileFor(
            const std::vector<std::pair<std::size_t, std::uint32_t>>
                &offeredTiles,
            const voxel::VoxelPosition position,
            const std::size_t side,
            const std::uint32_t seed)
        {
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

            auto rollValue = choiceRollFor(position, side, seed) % total;
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

            return likedTile;
        }

        void layPlacements(DecorWeaveState &weave)
        {
            for (std::size_t index = 0; index < weave.faces.size(); ++index)
            {
                const auto looking = gfx::Vec3(
                    voxelmap::getFaceNormal(weave.faces[index].side)).y;
                const auto upward = looking > 0.0F;

                if (looking < 0.0F || weave.stampedTiles.contains(index))
                {
                    continue;
                }

                wfc::Domain mayDomain(weave.decor.size() + 1);
                std::vector<std::pair<std::size_t, std::uint32_t>>
                    offeredTiles;

                for (std::size_t which = 0; which < weave.decor.size();
                     ++which)
                {
                    const auto &record = weave.decor[which];
                    const auto fits =
                        std::find(
                            record.allowedBaseTiles.begin(),
                            record.allowedBaseTiles.end(),
                            weave.drawnTiles[index])
                        != record.allowedBaseTiles.end();

                    if (fits && !isDecorSpanned(record)
                        && record.tile.atlas
                               == weave.drawnTiles[index].atlas
                        && frequencyRollFor(
                               weave.faces[index].cell.position,
                               which,
                               weave.seed,
                               upward ? 0U
                                      : static_cast<std::uint32_t>(
                                            weave.faces[index].side + 1))
                               < record.frequency)
                    {
                        offeredTiles.emplace_back(which, record.weight);
                    }
                    else
                    {
                        mayDomain.remove(weave.order[which]);
                    }
                }

                if (offeredTiles.empty())
                {
                    continue;
                }

                const auto likedTile = likedTileFor(
                    offeredTiles,
                    weave.faces[index].cell.position,
                    weave.faces[index].side,
                    weave.seed);

                weave.preferences.push_back(weave.order[likedTile]);

                if (upward)
                {
                    weave.byGround.emplace(
                        std::pair{
                            weave.faces[index].cell.position.x,
                            weave.faces[index].cell.position.z},
                        weave.decorPlacements.size());
                }
                else
                {
                    weave.byWall.emplace(
                        std::pair{
                            weave.faces[index].side,
                            weave.faces[index].cell.position},
                        weave.decorPlacements.size());
                }

                weave.decorPlacements.push_back(
                    DecorPlacement{
                        .face = index,
                        .position = weave.faces[index].cell.position,
                        .side = weave.faces[index].side});
                weave.waveDomains.push_back(mayDomain);
            }
        }

        [[nodiscard]] const wfc::CompatibilityTable &seamTableFor(
            const wfc::CompatibilityTable &interiorTable,
            const wfc::CompatibilityTable &boundaryTable,
            const voxel::VoxelPosition minePosition,
            const voxel::VoxelPosition otherPosition,
            std::int32_t voxel::VoxelPosition::*axis)
        {
            const auto sameCube = voxel::cubeCornerOf(minePosition).*axis
                               == voxel::cubeCornerOf(otherPosition).*axis;

            return sameCube ? interiorTable : boundaryTable;
        }

        void layAdjacencies(DecorWeaveState &weave, const SeamTables &tables)
        {
            for (std::size_t index = 0;
                 index < weave.decorPlacements.size();
                 ++index)
            {
                const auto &mine = weave.decorPlacements[index].position;

                if (weave.decorPlacements[index].side != voxelmap::kTopSide)
                {
                    const auto way =
                        getWallTangent(weave.decorPlacements[index].side);
                    const auto sideways = voxel::VoxelPosition{
                        .x = mine.x + way.x,
                        .y = mine.y,
                        .z = mine.z + way.z};
                    const auto underPosition = voxel::VoxelPosition{
                        .x = mine.x, .y = mine.y - 1, .z = mine.z};
                    const auto acrossWall = weave.byWall.find(
                        std::pair{
                            weave.decorPlacements[index].side, sideways});
                    const auto belowWall = weave.byWall.find(
                        std::pair{
                            weave.decorPlacements[index].side,
                            underPosition});

                    if (acrossWall != weave.byWall.end())
                    {
                        weave.adjacencies.emplace_back(
                            index,
                            acrossWall->second,
                            seamTableFor(
                                tables.horizontalInteriorTable,
                                tables.horizontalBoundaryTable,
                                mine,
                                sideways,
                                way.x != 0 ? &voxel::VoxelPosition::x
                                           : &voxel::VoxelPosition::z));
                    }

                    if (belowWall != weave.byWall.end())
                    {
                        weave.adjacencies.emplace_back(
                            index,
                            belowWall->second,
                            seamTableFor(
                                tables.verticalInteriorTable,
                                tables.verticalBoundaryTable,
                                mine,
                                underPosition,
                                &voxel::VoxelPosition::y));
                    }

                    continue;
                }

                const auto east = weave.byGround.find(
                    std::pair{mine.x + 1, mine.z});
                const auto south = weave.byGround.find(
                    std::pair{mine.x, mine.z + 1});

                if (east != weave.byGround.end()
                    && weave.decorPlacements[east->second].position.y
                           == mine.y)
                {
                    weave.adjacencies.emplace_back(
                        index,
                        east->second,
                        seamTableFor(
                            tables.horizontalInteriorTable,
                            tables.horizontalBoundaryTable,
                            mine,
                            weave.decorPlacements[east->second].position,
                            &voxel::VoxelPosition::x));
                }

                if (south != weave.byGround.end()
                    && weave.decorPlacements[south->second].position.y
                           == mine.y)
                {
                    weave.adjacencies.emplace_back(
                        index,
                        south->second,
                        seamTableFor(
                            tables.verticalInteriorTable,
                            tables.verticalBoundaryTable,
                            mine,
                            weave.decorPlacements[south->second].position,
                            &voxel::VoxelPosition::z));
                }
            }
        }

        [[nodiscard]] std::map<std::size_t, tilemap::Tile> getDecoratedStamps(
            const DecorWeaveState &weave,
            const std::vector<std::size_t> &assignment)
        {
            auto updatedStamps = weave.stampedTiles;

            for (std::size_t index = 0;
                 index < weave.decorPlacements.size();
                 ++index)
            {
                const auto value = assignment[index];
                const auto which = value < weave.blank
                                 ? weave.placeOf[value]
                                 : weave.blank;

                if (which != weave.blank)
                {
                    updatedStamps.emplace(
                        weave.decorPlacements[index].face,
                        weave.decor[which].tile);
                }
            }

            return updatedStamps;
        } // GCOVR_EXCL_LINE
    }

    std::vector<std::pair<std::size_t, std::map<std::size_t, tilemap::Tile>>>
    getSolveDecorLayers(
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

            auto placedDecor = getSolveDecor(
                faces,
                drawnTiles,
                ownTiles,
                decorRules,
                seed
                    ^ getHashMix(
                        static_cast<std::uint32_t>(layer) | 1U));

            if (!placedDecor.empty())
            {
                layerGroups.emplace_back(layer, std::move(placedDecor));
            }
        }

        return layerGroups;
    } // GCOVR_EXCL_LINE

    std::map<std::size_t, tilemap::Tile> getSolveDecor(
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

        DecorWeaveState weave{
            .faces = faces,
            .drawnTiles = drawnTiles,
            .decor = decor,
            .decorRules = decorRules,
            .seed = seed,
            .blank = decor.size()};

        weave.stampedTiles =
            getPlaceSpannedDecor(faces, drawnTiles, decor, seed);

        layShuffledOrder(weave);
        layPlacements(weave);

        if (weave.decorPlacements.empty())
        {
            return weave.stampedTiles;
        }

        const auto meets = [&weave](
                               const std::size_t one,
                               const voxel::Side side,
                               const voxel::EdgeKind kind,
                               const std::size_t other)
        {
            if (one == weave.blank || other == weave.blank)
            {
                return true;
            }

            return isSeamCompatible(
                weave.decorRules,
                weave.decor[weave.placeOf[one]].tile,
                side,
                kind,
                weave.decor[weave.placeOf[other]].tile);
        };

        const auto tables =
            seamTables(decor.size() + 1, meets);

        layAdjacencies(weave, tables);

        std::vector<std::reference_wrapper<const wfc::IConstraint>>
            constraints(
                weave.adjacencies.begin(), weave.adjacencies.end());

        const auto solution =
            wfc::Solver(
                std::move(weave.waveDomains),
                std::move(constraints),
                {},
                wfc::SolverLimits{.maxSteps = kMaxSolveSteps},
                std::move(weave.preferences))
                .getSolveResult();

        if (solution.outcome != wfc::SolveOutcome::Solved)
        {
            return weave.stampedTiles;
        }

        return getDecoratedStamps(weave, solution.assignment);
    } // GCOVR_EXCL_LINE

}
