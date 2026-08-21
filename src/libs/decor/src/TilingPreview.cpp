#include <functional>
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

    namespace
    {
        [[nodiscard]] std::vector<tilemap::Tile> spokenTiles(
            const tile::TileRules &rules, const tilemap::Tile middleTile)
        {
            std::set<tilemap::Tile> tiles{middleTile};

            for (const auto &rule : rules.allRules())
            {
                tiles.insert(rule.tile);
                tiles.insert(
                    rule.allowedTiles.begin(), rule.allowedTiles.end());
            }

            return {tiles.begin(), tiles.end()};
        } // GCOVR_EXCL_LINE
    }

    std::optional<std::vector<std::optional<tilemap::Tile>>>
    previewNeighbourhood(
        const tile::TileRules &rules,
        const tilemap::Tile middleTile,
        const std::size_t side,
        const std::uint32_t seed)
    {
        const auto spokenTileSet = spokenTiles(rules, middleTile);
        const auto order = shuffledValues(spokenTileSet.size() + 1, seed);
        std::vector<std::optional<tilemap::Tile>> alphabetTiles(
            spokenTileSet.size() + 1);

        for (std::size_t index = 0; index < spokenTileSet.size(); ++index)
        {
            alphabetTiles[order[index]] = spokenTileSet[index];
        }

        const auto meets = [&alphabetTiles, &rules](
                               const std::size_t one,
                               const voxel::Side side,
                               const voxel::EdgeKind kind,
                               const std::size_t other)
        {
            const auto &oneTile = alphabetTiles[one];
            const auto &otherTile = alphabetTiles[other];

            if (oneTile.has_value() && otherTile.has_value())
            {
                return seamCompatible(rules, *oneTile, side, kind, *otherTile);
            }

            const auto rims = [&rules, kind](
                                  const std::optional<tilemap::Tile> &tile,
                                  const voxel::Side side)
            {
                return !tile.has_value()
                       || rules.allowsBoundary(
                           *tile,
                           tilemap::TileEdge{
                               .side = side, .edge = kind});
            };

            return rims(oneTile, side) && rims(otherTile, voxel::facing(side));
        };

        const auto tables =
            seamTables(alphabetTiles.size(), meets);

        std::size_t pinnedCount = 0;

        for (std::size_t index = 0; index < alphabetTiles.size(); ++index)
        {
            if (alphabetTiles[index] == middleTile)
            {
                pinnedCount = index;
            }
        }

        std::vector<wfc::Domain> waveDomains;

        waveDomains.reserve(side * side);

        for (std::size_t index = 0; index < side * side; ++index)
        {
            waveDomains.push_back(
                index == ((side / 2) * side) + (side / 2)
                       ? wfc::Domain::singleton(pinnedCount,
                           alphabetTiles.size())
                       : wfc::Domain(alphabetTiles.size()));
        }

        std::vector<wfc::AdjacencyConstraint> adjacencies;

        for (std::size_t row = 0; row < side; ++row)
        {
            for (std::size_t column = 0; column < side; ++column)
            {
                const auto cellIndex = (row * side) + column;

                if (column + 1 < side)
                {
                    adjacencies.emplace_back(
                        cellIndex,
                        cellIndex + 1,
                        column % voxel::kCubeSide == 0
                            ? tables.horizontalInteriorTable
                            : tables.horizontalBoundaryTable);
                }

                if (row + 1 < side)
                {
                    adjacencies.emplace_back(
                        cellIndex,
                        cellIndex + side,
                        row % voxel::kCubeSide == 0
                                                ? tables.verticalInteriorTable
                                                : tables.verticalBoundaryTable);
                }
            }
        }

        std::vector<std::reference_wrapper<const wfc::IConstraint>>
            constraints(adjacencies.begin(), adjacencies.end());

        const auto solution =
            wfc::Solver(
                std::move(waveDomains),
                std::move(constraints),
                {},
                wfc::SolverLimits{.maxSteps = kMaxSolveSteps})
                .solve();

        if (solution.outcome != wfc::SolveOutcome::Solved)
        {
            return std::nullopt;
        }

        std::vector<std::optional<tilemap::Tile>> tiles;

        tiles.reserve(solution.assignment.size());

        for (const auto value : solution.assignment)
        {
            tiles.push_back(alphabetTiles[value]);
        }

        return tiles;
    } // GCOVR_EXCL_LINE

}
