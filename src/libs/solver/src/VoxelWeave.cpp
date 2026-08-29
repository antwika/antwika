#include "antwika/solver/VoxelWeave.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <set>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/IConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>

#include "VoxelWeaveDetail.hpp"

namespace antwika::solver
{
    using namespace weavedetail;

    namespace
    {

        [[nodiscard]] std::vector<voxelmap::FaceRef> thinnestOf(
            const std::vector<voxelmap::FaceRef> &faces,
            std::vector<std::pair<std::size_t, std::size_t>> countedPairs)
        {
            std::sort(countedPairs.begin(), countedPairs.end());

            std::vector<voxelmap::FaceRef> conflictFaces;

            for (const auto &[room, which] : countedPairs)
            {
                if (conflictFaces.size() >= kMaxReportedConflicts)
                {
                    break;
                }

                conflictFaces.push_back(faces[which]);
            }

            return conflictFaces;
        } // GCOVR_EXCL_LINE

        using TileCorners = std::vector<std::pair<voxel::Corner, bool>>;

        using WantKey = std::tuple<
            DomainKey, voxel::VoxelPosition, std::size_t, voxel::StairHalf>;

        using TableKey = std::tuple<
            tilemap::TileEdge,
            tilemap::TileEdge,
            std::size_t,
            std::size_t>;

        struct WeaveState final
        {
            const std::vector<voxelmap::FaceRef> &faces;
            const tile::TileRules &rules;
            CornerSeams corners;

            std::vector<tilemap::Tile> keptTiles{};
            std::vector<std::size_t> cellOf{};
            std::vector<std::size_t> faceOf{};
            std::vector<wfc::Domain> waveDomains{};
            std::vector<std::size_t> domainIdOf{};
            std::vector<wfc::AdjacencyConstraint> adjacencies{};
            std::map<tilemap::Tile, TileCorners> cornersByTile{};

            std::size_t skippedFaceCount = 0;
            std::size_t settledCount = 0;
        };

        [[nodiscard]] const TileCorners &cornersFor(
            WeaveState &weave, const tilemap::Tile tile)
        {
            const auto foundCorners = weave.cornersByTile.find(tile);

            if (foundCorners != weave.cornersByTile.end())
            {
                return foundCorners->second;
            }

            return weave.cornersByTile
                .emplace(tile, weave.rules.cornersOf(tile))
                .first->second;
        }

        [[nodiscard]] std::vector<std::size_t> keptValuesFor(
            WeaveState &weave,
            const std::map<voxelmap::FaceRef, std::size_t> &standing,
            const std::set<tilemap::Tile> &wantedTiles,
            const voxelmap::FaceRef &face,
            const tilemap::Atlas atlas)
        {
            const auto edges = edgesOf(standing, weave.faces, face);
            const auto beyondCorners =
                getCornersBeyond(standing, weave.faces, face);

            std::vector<std::size_t> keptValues;

            keptValues.reserve(wantedTiles.size());

            for (const auto tile : wantedTiles)
            {
                if (tile.atlas != atlas)
                {
                    continue;
                }

                auto anyBarred = false;

                for (const auto &[edge, atRim] : edges)
                {
                    if (atRim ? !weave.rules.allowsBoundary(tile, edge)
                              : weave.rules.isBoundaryOnly(tile, edge))
                    {
                        anyBarred = true;
                        break;
                    }
                }

                if (!anyBarred)
                {
                    for (const auto &[corner, cornerFilled] :
                         cornersFor(weave, tile))
                    {
                        if (beyondCorners.at(corner) != cornerFilled)
                        {
                            anyBarred = true;
                            break;
                        }
                    }
                }

                if (!anyBarred)
                {
                    keptValues.push_back(getTileToIndex(tile));
                }
            }

            return keptValues;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::optional<TileSolve> layFaceDomains(
            WeaveState &weave)
        {
            const auto tilesByDomain = getRuledTilesByDomain(weave.rules);
            const auto standing = getFacesByPlace(weave.faces);

            std::map<WantKey, std::set<tilemap::Tile>> tilesByWant;

            for (std::size_t which = 0; which < weave.faces.size(); ++which)
            {
                const auto atlas = atlasOf(weave.faces[which].side);
                const auto wantedKey = DomainKey{atlas,
                    weave.faces[which].cell.material.kind};

                if (!tilesByDomain.contains(wantedKey))
                {
                    ++weave.skippedFaceCount;
                    continue;
                }

                const auto &offeredTiles = tilesByDomain.at(wantedKey);
                const WantKey wantKey{
                    wantedKey,
                    weave.faces[which].climbPosition,
                    weave.faces[which].side,
                    weave.faces[which].levelHalf};
                auto foundWanted = tilesByWant.find(wantKey);

                if (foundWanted == tilesByWant.end())
                {
                    foundWanted =
                        tilesByWant
                            .emplace(
                                wantKey,
                                tilesFor(
                                    weave.rules,
                                    offeredTiles,
                                    weave.faces[which]))
                            .first;
                }

                const auto &mine = foundWanted->second;

                if (mine.empty())
                {
                    ++weave.skippedFaceCount;
                    continue;
                }

                if (mine.size() == 1 && mine.size() < offeredTiles.size())
                {
                    weave.keptTiles[which] = *mine.begin();
                    ++weave.settledCount;
                    continue;
                }

                const auto keptValues = keptValuesFor(
                    weave, standing, mine, weave.faces[which], atlas);

                if (keptValues.empty())
                {
                    return TileSolve{
                        .troubleFailure = SolveFailure::EmptyDomain,
                        .unsatisfiedAtlas = atlas,
                        .skippedFaceCount = weave.skippedFaceCount,
                        .conflictFaces = {weave.faces[which]}};
                }

                auto domain = wfc::Domain::createSingleton(
                    keptValues.front(), kTileDomainSize);

                for (const auto value : keptValues)
                {
                    domain.add(value);
                }

                weave.cellOf[which] = weave.waveDomains.size();
                weave.faceOf.push_back(which);
                weave.waveDomains.push_back(domain);
            }

            return std::nullopt;
        }

        void layDomainIds(WeaveState &weave)
        {
            std::vector<wfc::Domain> distinctDomains;

            for (const auto &one : weave.waveDomains)
            {
                std::size_t sameId = 0;

                while (sameId < distinctDomains.size()
                       && !(distinctDomains[sameId] == one))
                {
                    ++sameId;
                }

                if (sameId == distinctDomains.size())
                {
                    distinctDomains.push_back(one);
                }

                weave.domainIdOf.push_back(sameId);
            }
        }

        [[nodiscard]] std::shared_ptr<const wfc::CompatibilityTable> tableFor(
            const WeaveState &weave, const FaceSeam &seam)
        {
            auto madeTable =
                std::make_shared<wfc::CompatibilityTable>(kTileDomainSize);
            auto anyPair = false;

            for (const auto one :
                 weave.waveDomains[weave.cellOf[seam.faceA]])
            {
                for (const auto other :
                     weave.waveDomains[weave.cellOf[seam.faceB]])
                {
                    const auto both = edgesCompatible(
                        weave.rules,
                        getTileFromIndex(one),
                        seam.edgeA,
                        getTileFromIndex(other),
                        seam.edgeB);

                    madeTable->set(one, other, both);
                    anyPair = anyPair || both;
                }
            }

            if (!anyPair)
            {
                return nullptr;
            }

            return madeTable;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::optional<TileSolve> layAdjacencies(
            WeaveState &weave)
        {
            std::map<TableKey, std::shared_ptr<const wfc::CompatibilityTable>>
                tablesByKey;

            for (const auto &seam :
                 getFaceAdjacency(weave.faces, weave.corners))
            {
                if (weave.cellOf[seam.faceA] == kNoFaceIndex
                    || weave.cellOf[seam.faceB] == kNoFaceIndex)
                {
                    continue;
                }

                const TableKey tableKey{
                    seam.edgeA,
                    seam.edgeB,
                    weave.domainIdOf[weave.cellOf[seam.faceA]],
                    weave.domainIdOf[weave.cellOf[seam.faceB]]};
                auto foundTable = tablesByKey.find(tableKey);

                if (foundTable == tablesByKey.end())
                {
                    auto madeTable = tableFor(weave, seam);

                    if (!madeTable)
                    {
                        return TileSolve{
                            .troubleFailure = SolveFailure::IncompatibleEdge,
                            .unsatisfiedEdge = seam.edgeA,
                            .skippedFaceCount = weave.skippedFaceCount,
                            .conflictFaces = {
                                weave.faces[seam.faceA],
                                weave.faces[seam.faceB]}};
                    }

                    foundTable =
                        tablesByKey.emplace(tableKey, std::move(madeTable))
                            .first;
                }

                weave.adjacencies.emplace_back(
                    weave.cellOf[seam.faceA],
                    weave.cellOf[seam.faceB],
                    foundTable->second);
            }

            return std::nullopt;
        }

        [[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>>
        getCountedRooms(const WeaveState &weave)
        {
            std::vector<std::pair<std::size_t, std::size_t>> countedPairs;

            for (std::size_t cell = 0; cell < weave.waveDomains.size();
                 ++cell)
            {
                std::size_t room = 0;

                for ([[maybe_unused]] const auto value :
                     weave.waveDomains[cell])
                {
                    ++room;
                }

                countedPairs.emplace_back(room, weave.faceOf[cell]);
            }

            return countedPairs;
        } // GCOVR_EXCL_LINE
    }

    std::size_t getTileToIndex(const tilemap::Tile tile)
    {
        return (tile.atlas == tilemap::Atlas::Floor ? kAtlasTiles : 0)
               + tile.index;
    }

    tilemap::Tile getTileFromIndex(const std::size_t value)
    {
        return tilemap::Tile{
            .atlas = value >= kAtlasTiles ? tilemap::Atlas::Floor
                   : tilemap::Atlas::Wall,
            .index = static_cast<std::uint16_t>(
                value % kAtlasTiles)};
    }

    std::optional<std::vector<tilemap::Tile>> getSolvedTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        const CornerSeams corners)
    {
        return getSolveTiles(faces, rules, corners).tiles;
    } // GCOVR_EXCL_LINE

    TileSolve getSolveTiles(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        const CornerSeams corners)
    {
        WeaveState weave{.faces = faces, .rules = rules, .corners = corners};

        weave.keptTiles = voxelmap::getDefaultTiles(faces);
        weave.cellOf.assign(faces.size(), kNoFaceIndex);

        if (const auto troubleFailure = layFaceDomains(weave))
        {
            return *troubleFailure;
        }

        if (weave.waveDomains.empty())
        {
            if (weave.settledCount > 0)
            {
                return TileSolve{
                    .tiles = weave.keptTiles,
                    .skippedFaceCount = weave.skippedFaceCount};
            }

            return TileSolve{
                .troubleFailure = SolveFailure::EmptyDomain,
                .unsatisfiedAtlas = tilemap::Atlas::Wall,
                .skippedFaceCount = weave.skippedFaceCount};
        }

        layDomainIds(weave);

        if (const auto troubleFailure = layAdjacencies(weave))
        {
            return *troubleFailure;
        }

        const auto countedPairs = getCountedRooms(weave);

        std::vector<std::reference_wrapper<const wfc::IConstraint>>
            constraints;

        constraints.reserve(weave.adjacencies.size());

        for (const auto &one : weave.adjacencies)
        {
            constraints.emplace_back(one);
        }

        const wfc::Solver solver(
            std::move(weave.waveDomains),
            std::move(constraints),
            {},
            wfc::SolverLimits{.maxSteps = kMaxSteps});
        const auto solution = solver.getSolveResult();

        if (solution.outcome != wfc::SolveOutcome::Solved)
        {
            return TileSolve{
                .troubleFailure = SolveFailure::Unsatisfiable,
                .skippedFaceCount = weave.skippedFaceCount,
                .conflictFaces = thinnestOf(faces, countedPairs)};
        }

        auto woven = weave.keptTiles;

        for (std::size_t cell = 0; cell < solution.assignment.size();
             ++cell)
        {
            woven[weave.faceOf[cell]] =
                getTileFromIndex(solution.assignment[cell]);
        }

        return TileSolve{
            .tiles = woven,
            .skippedFaceCount = weave.skippedFaceCount};
    } // GCOVR_EXCL_LINE

}
