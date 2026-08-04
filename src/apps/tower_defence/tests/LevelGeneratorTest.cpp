#include <bit>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/LevelError.hpp"
#include "antwika/tower_defence/LevelGenerator.hpp"
#include "antwika/tower_defence/LevelTile.hpp"

using antwika::tower_defence::campaignLevels;
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

// Both counts are chosen by tests/CMakeLists.txt.
// A build naming neither gets the wide sweep, not the cheap one.
#ifndef ANTWIKA_LEVEL_SEED_COUNT
#define ANTWIKA_LEVEL_SEED_COUNT 40
#endif

#ifndef ANTWIKA_LEVEL_PROPERTY_SEED_COUNT
#define ANTWIKA_LEVEL_PROPERTY_SEED_COUNT 40
#endif

#ifndef ANTWIKA_CAMPAIGN_SEED_COUNT
#define ANTWIKA_CAMPAIGN_SEED_COUNT 8
#endif

namespace
{
    // How many seeds the linearity property is asserted over.
    constexpr std::uint64_t kSeedCount = ANTWIKA_LEVEL_SEED_COUNT;

    // What a case reading some other property off a level takes.
    // The soak above is what reaches every branch of the generator.
    // So these state their assertion and leave the width to it.
    constexpr std::uint64_t kPropertySeedCount =
        ANTWIKA_LEVEL_PROPERTY_SEED_COUNT;

    // What each of the shipped campaign's own grids takes.
    // There are as many generations here as there are levels.
    // So this is the count that stays small under instrumentation.
    constexpr std::uint64_t kCampaignSeedCount =
        ANTWIKA_CAMPAIGN_SEED_COUNT;

    static_assert(
        kPropertySeedCount <= kSeedCount,
        "property cases read their levels off the soak's pool");

    // The soak's levels, generated once and shared by every case.
    // Generating is the expensive half of any one of these cases.
    // Regenerating one pool per case tripled the suite for nothing.
    const std::vector<Level> &sweptLevels()
    {
        static const std::vector<Level> levels = []
        {
            std::vector<Level> generated;
            generated.reserve(kSeedCount);
            for (std::uint64_t seed = 0; seed < kSeedCount; ++seed)
            {
                generated.push_back(generateLevel({.seed = seed}));
            }
            return generated;
        }();
        return levels;
    }

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
            SCOPED_TRACE("seed " + std::to_string(seed));
            expectSingleSimplePath(sweptLevels()[seed]);
        }
    }

    TEST(LevelGeneratorTest, EveryOpenSideMeetsAnOpenSide)
    {
        for (std::uint64_t seed = 0; seed < kPropertySeedCount; ++seed)
        {
            const Level &level = sweptLevels()[seed];
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
        const std::vector<Cell> &reference = sweptLevels()[0].path;
        bool sawADifferentPath = false;
        for (std::uint64_t seed = 1; seed < kPropertySeedCount; ++seed)
        {
            sawADifferentPath = sawADifferentPath
                || sweptLevels()[seed].path != reference;
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

    // The End tile is pinned to the last column.
    // So that column is never walled, whatever the spacing divides.
    // Width 7 with the default spacing of 3 is the case that asks.
    // Column 6 is a multiple of 3 and is also the End's column.
    // A wall there leaves the End in a column of one open row.
    // The walk out of Start need not be able to reach that row.
    TEST(LevelGeneratorTest, TheEndColumnIsNeverWalled)
    {
        for (std::uint64_t seed = 0; seed < kPropertySeedCount; ++seed)
        {
            const Level level = generateLevel(
                {.width = 7, .height = 5, .seed = seed});
            SCOPED_TRACE("seed " + std::to_string(seed));
            expectSingleSimplePath(level);
            EXPECT_EQ(level.path.back().x, 6U);
        }
    }

    // A campaign's levels are not all the same grid.
    // The linear path is a property of the alphabet, not of a size.
    // So every shipped level has to hold it too.
    // A new level added to campaignLevels() is soaked here for free.
    TEST(LevelGeneratorTest, EveryShippedLevelIsGenerableAndLinear)
    {
        for (const auto &plan : campaignLevels())
        {
            for (std::uint64_t seed = 0; seed < kCampaignSeedCount;
                 ++seed)
            {
                LevelConfig level = plan.level;
                level.seed = seed;

                SCOPED_TRACE(
                    "a " + std::to_string(level.width) + "x"
                    + std::to_string(level.height) + " grid walled every "
                    + std::to_string(level.wallSpacing)
                    + ", seed " + std::to_string(seed));
                expectSingleSimplePath(generateLevel(level));
            }
        }
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

    TEST(LevelGeneratorTest, AZeroInitialBudgetIsRefused)
    {
        EXPECT_THROW(
            static_cast<void>(
                generateLevel({.initialSolverSteps = 0})),
            LevelError);
    }

    TEST(LevelGeneratorTest, AnInitialBudgetAboveTheCapIsRefused)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel(
                {.initialSolverSteps = 2, .maxSolverSteps = 1})),
            LevelError);
    }

    TEST(LevelGeneratorTest, ExhaustingTheSolverBudgetIsReported)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel(
                {.initialSolverSteps = 1,
                 .maxSolverSteps = 1,
                 .maxAttempts = 2})),
            LevelError);
    }

    // No 32x32 grid collapses within four steps, whatever the seed.
    // So every attempt fails and the doubling walks up to its cap.
    // A budget still capped after that walk is what this asserts.
    // Batches of eight take the budget 1, 2, 4 and then hold it.
    TEST(LevelGeneratorTest, AnEscalatingBudgetIsStillBounded)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel(
                {.width = 32,
                 .height = 32,
                 .initialSolverSteps = 1,
                 .maxSolverSteps = 4,
                 .maxAttempts = 32})),
            LevelError);
    }
} // namespace
