#include "antwika/solver/VoxelWeave.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>

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
        const auto tilesByDomain = getRuledTilesByDomain(rules);
        const auto standing = getFacesByPlace(faces);
        auto keptTiles = voxelmap::getDefaultTiles(faces);

        std::vector<std::size_t> cellOf(faces.size(), kNoFaceIndex);
        std::vector<std::size_t> faceOf;
        std::vector<wfc::Domain> waveDomains;
        std::size_t skippedFaceCount = 0;
        std::size_t settledCount = 0;

        for (std::size_t which = 0; which < faces.size(); ++which)
        {
            const auto atlas = atlasOf(faces[which].side);
            const auto wantedKey = DomainKey{atlas,
                faces[which].cell.material.kind};

            if (!tilesByDomain.contains(wantedKey))
            {
                ++skippedFaceCount;
                continue;
            }

            const auto &offeredTiles = tilesByDomain.at(wantedKey);
            const auto mine = tilesFor(rules, offeredTiles, faces[which]);

            if (mine.empty())
            {
                ++skippedFaceCount;
                continue;
            }

            if (mine.size() == 1 && mine.size() < offeredTiles.size())
            {
                keptTiles[which] = *mine.begin();
                ++settledCount;
                continue;
            }

            wfc::Domain domain(kTileDomainSize);
            const auto edges = edgesOf(standing, faces, faces[which]);
            const auto beyondCorners =
                getCornersBeyond(standing, faces, faces[which]);

            for (std::size_t value = 0; value < kTileDomainSize;
                 ++value)
            {
                const auto tile = getTileFromIndex(value);

                if (tile.atlas != atlas || !mine.contains(tile))
                {
                    domain.remove(value);
                    continue;
                }

                auto anyBarred = false;

                for (const auto &[edge, atRim] : edges)
                {
                    if (atRim ? !rules.allowsBoundary(tile, edge)
                              : rules.isBoundaryOnly(tile, edge))
                    {
                        anyBarred = true;
                        break;
                    }
                }

                for (const auto &[corner, cornerFilled] :
                     rules.cornersOf(tile))
                {
                    if (beyondCorners.at(corner) != cornerFilled)
                    {
                        anyBarred = true;
                        break;
                    }
                }

                if (anyBarred)
                {
                    domain.remove(value);
                }
            }

            if (domain.isEmpty())
            {
                return TileSolve{
                    .troubleFailure = SolveFailure::EmptyDomain,
                    .unsatisfiedAtlas = atlas,
                    .skippedFaceCount = skippedFaceCount,
                    .conflictFaces = {faces[which]}};
            }

            cellOf[which] = waveDomains.size();
            faceOf.push_back(which);
            waveDomains.push_back(domain);
        }

        if (waveDomains.empty())
        {
            if (settledCount > 0)
            {
                return TileSolve{
                    .tiles = keptTiles, .skippedFaceCount = skippedFaceCount};
            }

            return TileSolve{
                .troubleFailure = SolveFailure::EmptyDomain,
                .unsatisfiedAtlas = tilemap::Atlas::Wall,
                .skippedFaceCount = skippedFaceCount};
        }

        std::vector<wfc::AdjacencyConstraint> adjacencies;

        for (const auto &seam : getFaceAdjacency(faces, corners))
        {
            if (cellOf[seam.faceA] == kNoFaceIndex
                || cellOf[seam.faceB] == kNoFaceIndex)
            {
                continue;
            }

            wfc::CompatibilityTable table(kTileDomainSize);
            auto anyPair = false;

            for (const auto one : waveDomains[cellOf[seam.faceA]])
            {
                for (const auto other : waveDomains[cellOf[seam.faceB]])
                {
                    const auto both = edgesCompatible(
                        rules,
                        getTileFromIndex(one),
                        seam.edgeA,
                        getTileFromIndex(other),
                        seam.edgeB);

                    table.set(one, other, both);
                    anyPair = anyPair || both;
                }
            }

            if (!anyPair)
            {
                return TileSolve{
                    .troubleFailure = SolveFailure::IncompatibleEdge,
                    .unsatisfiedEdge = seam.edgeA,
                    .skippedFaceCount = skippedFaceCount,
                    .conflictFaces = {
                        faces[seam.faceA], faces[seam.faceB]}};
            }

            adjacencies.emplace_back(
                cellOf[seam.faceA],
                cellOf[seam.faceB],
                std::move(table));
        }

        std::vector<std::pair<std::size_t, std::size_t>> countedPairs;

        for (std::size_t cell = 0; cell < waveDomains.size(); ++cell)
        {
            std::size_t room = 0;

            for ([[maybe_unused]] const auto value : waveDomains[cell])
            {
                ++room;
            }

            countedPairs.emplace_back(room, faceOf[cell]);
        }

        std::vector<std::reference_wrapper<const wfc::IConstraint>>
            constraints;

        constraints.reserve(adjacencies.size());

        for (const auto &one : adjacencies)
        {
            constraints.emplace_back(one);
        }

        const wfc::Solver solver(
            std::move(waveDomains),
            std::move(constraints),
            {},
            wfc::SolverLimits{.maxSteps = kMaxSteps});
        const auto solution = solver.getSolve();

        if (solution.outcome != wfc::SolveOutcome::Solved)
        {
            return TileSolve{
                .troubleFailure = SolveFailure::Unsatisfiable,
                .skippedFaceCount = skippedFaceCount,
                .conflictFaces = thinnestOf(faces, countedPairs)};
        }

        auto woven = keptTiles;

        for (std::size_t cell = 0; cell < solution.assignment.size();
             ++cell)
        {
            woven[faceOf[cell]] = getTileFromIndex(solution.assignment[cell]);
        }

        return TileSolve{
            .tiles = woven,
            .skippedFaceCount = skippedFaceCount};
    } // GCOVR_EXCL_LINE

}
