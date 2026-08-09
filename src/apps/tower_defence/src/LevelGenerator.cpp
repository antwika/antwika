#include "antwika/tower_defence/LevelGenerator.hpp"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include <antwika/rng/SplitMix64Rng.hpp>
#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/IConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>

#include "antwika/tower_defence/LevelError.hpp"
#include "antwika/tower_defence/LevelTile.hpp"

namespace antwika::tower_defence
{

    namespace
    {
        using antwika::rng::SplitMix64Rng;
        using antwika::wfc::AdjacencyConstraint;
        using antwika::wfc::CompatibilityTable;
        using antwika::wfc::Domain;
        using antwika::wfc::IConstraint;
        using antwika::wfc::SolveOutcome;
        using antwika::wfc::SolveResult;
        using antwika::wfc::Solver;

        constexpr std::uint64_t kAttemptStride = 0x9E3779B97F4A7C15ULL;

        constexpr std::uint32_t kAttemptsPerBudget = 8;

        std::size_t indexOf(
            const std::uint32_t width, const Cell &cell)
        {
            return static_cast<std::size_t>(cell.y) * width + cell.x;
        }

        bool isWallColumn(const LevelConfig &config, const std::uint32_t x)
        {
            return x % config.wallSpacing == 0 && x != 0
                && x + 1 < config.width;
        }

        bool leavesGrid(
            const Tile tile, const Cell &cell, const LevelConfig &config)
        {
            return (isOpen(tile, Side::North) && cell.y == 0)
                || (isOpen(tile, Side::South)
                    && cell.y + 1 == config.height)
                || (isOpen(tile, Side::West) && cell.x == 0)
                || (isOpen(tile, Side::East)
                    && cell.x + 1 == config.width);
        }

        Cell neighbour(const Cell &cell, const Side side)
        {
            const auto bits = static_cast<std::uint32_t>(side);
            const std::uint32_t north = bits & 1U;
            const std::uint32_t east = (bits >> 1) & 1U;
            const std::uint32_t south = (bits >> 2) & 1U;
            const std::uint32_t west = (bits >> 3) & 1U;
            return Cell{
                .x = cell.x + east - west, .y = cell.y + south - north};
        }

        struct Layout final
        {
            Cell start;
            Cell end;
            std::vector<Domain> wave;
        };

        Layout buildLayout(
            const LevelConfig &config, const std::uint64_t seed)
        {
            SplitMix64Rng rng(seed);
            const auto startRow =
                static_cast<std::uint32_t>(rng.next() % config.height);
            const auto endRow =
                static_cast<std::uint32_t>(rng.next() % config.height);

            std::vector<std::uint32_t> gapRow(config.width, 0);
            for (std::uint32_t x = 0; x < config.width; ++x)
            {
                if (isWallColumn(config, x))
                {
                    gapRow[x] = static_cast<std::uint32_t>(
                        rng.next() % config.height);
                }
            }

            Layout layout{
                .start = Cell{.x = 0, .y = startRow},
                .end = Cell{.x = config.width - 1, .y = endRow},
                .wave = {}};
            layout.wave.reserve(
                static_cast<std::size_t>(config.width) * config.height);

            constexpr auto kStart = static_cast<std::size_t>(Tile::Start);
            constexpr auto kEnd = static_cast<std::size_t>(Tile::End);

            for (std::uint32_t y = 0; y < config.height; ++y)
            {
                for (std::uint32_t x = 0; x < config.width; ++x)
                {
                    const Cell cell{.x = x, .y = y};
                    if (cell == layout.start)
                    {
                        layout.wave.push_back(
                            Domain::singleton(kStart, kTileCount));
                        continue;
                    }
                    if (cell == layout.end)
                    {
                        layout.wave.push_back(
                            Domain::singleton(kEnd, kTileCount));
                        continue;
                    }
                    if (isWallColumn(config, x) && y != gapRow[x])
                    {
                        layout.wave.push_back(Domain::singleton(
                            static_cast<std::size_t>(Tile::Empty),
                            kTileCount));
                        continue;
                    }

                    Domain domain(kTileCount);
                    domain.remove(kStart);
                    domain.remove(kEnd);
                    for (std::size_t v = 0; v < kTileCount; ++v)
                    {
                        if (domain.contains(v)
                            && leavesGrid(
                                tileFromSymbol(v), cell, config))
                        {
                            domain.remove(v);
                        }
                    }
                    layout.wave.push_back(std::move(domain));
                }
            }

            return layout;
        }

        CompatibilityTable sideTable(const Side from, const Side to)
        {
            CompatibilityTable table(kTileCount);
            for (std::size_t left = 0; left < kTileCount; ++left)
            {
                for (std::size_t right = 0; right < kTileCount; ++right)
                {
                    table.set(
                        left,
                        right,
                        isOpen(tileFromSymbol(left), from)
                            == isOpen(tileFromSymbol(right), to));
                }
            }
            return table;
        } // GCOVR_EXCL_LINE

        std::vector<AdjacencyConstraint> buildConstraints(
            const LevelConfig &config,
            const CompatibilityTable &horizontal,
            const CompatibilityTable &vertical)
        {
            std::vector<AdjacencyConstraint> constraints;
            constraints.reserve(
                static_cast<std::size_t>(config.width) * config.height
                * 2);
            for (std::uint32_t y = 0; y < config.height; ++y)
            {
                for (std::uint32_t x = 0; x < config.width; ++x)
                {
                    const Cell cell{.x = x, .y = y};
                    const std::size_t here = indexOf(config.width, cell);
                    if (x + 1 < config.width)
                    {
                        constraints.emplace_back(
                            here,
                            indexOf(
                                config.width,
                                neighbour(cell, Side::East)),
                            horizontal);
                    }
                    if (y + 1 < config.height)
                    {
                        constraints.emplace_back(
                            here,
                            indexOf(
                                config.width,
                                neighbour(cell, Side::South)),
                            vertical);
                    }
                }
            }
            return constraints;
        } // GCOVR_EXCL_LINE

        Level buildLevel(
            const LevelConfig &config,
            const Layout &layout,
            const std::vector<std::size_t> &assignment)
        {
            std::vector<Tile> solved;
            solved.reserve(assignment.size());
            for (const std::size_t value : assignment)
            {
                solved.push_back(tileFromSymbol(value));
            }

            Level level{
                .width = config.width,
                .height = config.height,
                .tiles = std::vector<Tile>(
                    assignment.size(), Tile::Empty),
                .path = {}}; // GCOVR_EXCL_LINE

            Cell current = layout.start;
            std::uint8_t entryMask = 0;
            level.path.push_back(current);
            while (solved[indexOf(config.width, current)] != Tile::End)
            {
                const Tile tile = solved[indexOf(config.width, current)];
                Cell next = current;
                std::uint8_t nextEntry = 0;
                for (const Side side : kSides)
                {
                    const auto bit = static_cast<std::uint8_t>(side);
                    if (isOpen(tile, side) && (bit & entryMask) == 0)
                    {
                        next = neighbour(current, side);
                        nextEntry =
                            static_cast<std::uint8_t>(opposite(side));
                    }
                }
                current = next;
                entryMask = nextEntry;
                level.path.push_back(current);
            }

            for (const Cell &cell : level.path)
            {
                const std::size_t index = indexOf(config.width, cell);
                level.tiles[index] = solved[index];
            }
            return level;
        }
    }

    Level generateLevel(const LevelConfig &config)
    {
        if (config.width < 3)
        {
            throw LevelError("Level width must be at least 3");
        }
        if (config.height < 1)
        {
            throw LevelError("Level height must be at least 1");
        }
        if (config.wallSpacing < 2)
        {
            throw LevelError("Level wallSpacing must be at least 2");
        }
        if (config.initialSolverSteps < 1)
        {
            throw LevelError(
                "Level initialSolverSteps must be at least 1");
        }
        if (config.initialSolverSteps > config.maxSolverSteps)
        {
            throw LevelError(
                "Level initialSolverSteps must not exceed "
                "maxSolverSteps");
        }

        const CompatibilityTable horizontal =
            sideTable(Side::East, Side::West);
        const CompatibilityTable vertical =
            sideTable(Side::South, Side::North);
        const std::vector<AdjacencyConstraint> constraints =
            buildConstraints(config, horizontal, vertical);
        std::vector<std::reference_wrapper<const IConstraint>> references;
        references.reserve(constraints.size());
        for (const AdjacencyConstraint &constraint : constraints)
        {
            references.emplace_back(constraint);
        }

        std::uint64_t budget = config.initialSolverSteps;
        for (std::uint32_t attempt = 0; attempt < config.maxAttempts;
             ++attempt)
        {
            const std::uint64_t seed =
                config.seed
                ^ (static_cast<std::uint64_t>(attempt) * kAttemptStride);
            Layout layout = buildLayout(config, seed);
            const Solver solver(
                std::move(layout.wave),
                references,
                {},
                {.maxSteps = budget});
            const SolveResult result = solver.solve();
            if (result.outcome == SolveOutcome::Solved)
            {
                return buildLevel(config, layout, result.assignment);
            }

            if (attempt % kAttemptsPerBudget == kAttemptsPerBudget - 1)
            {
                budget = budget > config.maxSolverSteps / 2
                    ? config.maxSolverSteps
                    : budget * 2;
            }
        }

        throw LevelError("No level found within the attempt budget");
    }

}
