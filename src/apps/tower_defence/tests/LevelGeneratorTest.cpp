#include <bit>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/LevelError.hpp"
#include "antwika/tower_defence/LevelGenerator.hpp"
#include "antwika/tower_defence/LevelTile.hpp"

using antwika::tower_defence::Cell;
using antwika::tower_defence::generateLevel;
using antwika::tower_defence::isOpen;
using antwika::tower_defence::kSides;
using antwika::tower_defence::Level;
using antwika::tower_defence::LevelConfig;
using antwika::tower_defence::LevelError;
using antwika::tower_defence::openSides;
using antwika::tower_defence::Side;
using antwika::tower_defence::Tile;

namespace
{
    // How many seeds the linearity property is asserted over.
    constexpr std::uint64_t kSeedCount = 40;

    bool adjacent(const Cell &left, const Cell &right)
    {
        const auto dx = static_cast<std::int64_t>(left.x)
            - static_cast<std::int64_t>(right.x);
        const auto dy = static_cast<std::int64_t>(left.y)
            - static_cast<std::int64_t>(right.y);
        return (dx * dx) + (dy * dy) == 1;
    }

    // The whole point of the generator, asserted rather than assumed.
    // One start, one end, and a simple walk between them.
    // Not one cell of path anywhere else in the grid.
    void expectSingleSimplePath(const Level &level)
    {
        ASSERT_GE(level.path.size(), 2U);
        EXPECT_EQ(level.at(level.path.front()), Tile::Start);
        EXPECT_EQ(level.at(level.path.back()), Tile::End);

        std::set<std::pair<std::uint32_t, std::uint32_t>> seen;
        for (const Cell &cell : level.path)
        {
            EXPECT_TRUE(seen.emplace(cell.x, cell.y).second)
                << "cell visited twice: " << cell.x << "," << cell.y;
            EXPECT_LT(cell.x, level.width);
            EXPECT_LT(cell.y, level.height);
        }

        for (std::size_t i = 1; i < level.path.size(); ++i)
        {
            EXPECT_TRUE(adjacent(level.path[i - 1], level.path[i]))
                << "step " << i << " is not a single cell";
        }

        std::size_t starts = 0;
        std::size_t ends = 0;
        std::size_t occupied = 0;
        for (std::uint32_t y = 0; y < level.height; ++y)
        {
            for (std::uint32_t x = 0; x < level.width; ++x)
            {
                const Cell cell{.x = x, .y = y};
                const Tile tile = level.at(cell);
                if (tile == Tile::Empty)
                {
                    continue;
                }
                ++occupied;
                starts += tile == Tile::Start ? 1U : 0U;
                ends += tile == Tile::End ? 1U : 0U;

                // No cell may branch: two open sides is the maximum.
                EXPECT_LE(std::popcount(openSides(tile)), 2);
            }
        }

        EXPECT_EQ(starts, 1U);
        EXPECT_EQ(ends, 1U);
        EXPECT_EQ(occupied, level.path.size())
            << "the grid holds path cells the walk never reached";
    }

    TEST(LevelGeneratorTest, EverySeedGivesOneSimplePathFromStartToEnd)
    {
        for (std::uint64_t seed = 0; seed < kSeedCount; ++seed)
        {
            const Level level = generateLevel({.seed = seed});
            SCOPED_TRACE("seed " + std::to_string(seed));
            expectSingleSimplePath(level);
        }
    }

    TEST(LevelGeneratorTest, EveryOpenSideMeetsAnOpenSide)
    {
        for (std::uint64_t seed = 0; seed < 8; ++seed)
        {
            const Level level = generateLevel({.seed = seed});
            for (const Cell &cell : level.path)
            {
                const Tile tile = level.at(cell);
                for (const Side side : kSides)
                {
                    if (!isOpen(tile, side))
                    {
                        continue;
                    }
                    const std::int64_t nx =
                        static_cast<std::int64_t>(cell.x)
                        + (side == Side::East ? 1 : 0)
                        - (side == Side::West ? 1 : 0);
                    const std::int64_t ny =
                        static_cast<std::int64_t>(cell.y)
                        + (side == Side::South ? 1 : 0)
                        - (side == Side::North ? 1 : 0);
                    ASSERT_GE(nx, 0);
                    ASSERT_GE(ny, 0);
                    ASSERT_LT(nx, static_cast<std::int64_t>(level.width));
                    ASSERT_LT(
                        ny, static_cast<std::int64_t>(level.height));
                    const Cell other{
                        .x = static_cast<std::uint32_t>(nx),
                        .y = static_cast<std::uint32_t>(ny)};
                    EXPECT_TRUE(isOpen(
                        level.at(other),
                        antwika::tower_defence::opposite(side)));
                }
            }
        }
    }

    TEST(LevelGeneratorTest, TheSameSeedGivesTheSameLevel)
    {
        const Level first = generateLevel({.seed = 7});
        const Level second = generateLevel({.seed = 7});
        EXPECT_EQ(first.tiles, second.tiles);
        EXPECT_EQ(first.path, second.path);
    }

    TEST(LevelGeneratorTest, DifferentSeedsGiveDifferentLevels)
    {
        const std::vector<Cell> reference = generateLevel({.seed = 0}).path;
        bool sawADifferentPath = false;
        for (std::uint64_t seed = 1; seed < 8; ++seed)
        {
            sawADifferentPath = sawADifferentPath
                || generateLevel({.seed = seed}).path != reference;
        }
        EXPECT_TRUE(sawADifferentPath);
    }

    TEST(LevelGeneratorTest, ASmallGridStillProducesAPath)
    {
        const Level level =
            generateLevel({.width = 3, .height = 1, .seed = 1});
        expectSingleSimplePath(level);
        EXPECT_EQ(level.path.size(), 3U);
    }

    TEST(LevelGeneratorTest, ATooNarrowGridIsRefused)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel({.width = 2})),
            LevelError);
    }

    TEST(LevelGeneratorTest, AZeroHeightGridIsRefused)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel({.height = 0})),
            LevelError);
    }

    TEST(LevelGeneratorTest, AWallEveryColumnIsRefused)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel({.wallSpacing = 1})),
            LevelError);
    }

    TEST(LevelGeneratorTest, ExhaustingTheSolverBudgetIsReported)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel(
                {.maxSolverSteps = 1, .maxAttempts = 2})),
            LevelError);
    }
} // namespace
