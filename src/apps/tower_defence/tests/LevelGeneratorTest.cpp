#include <gtest/gtest.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

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
    constexpr std::uint64_t kSeedCount = ANTWIKA_LEVEL_SEED_COUNT;

    constexpr std::uint64_t kPropertySeedCount =
        ANTWIKA_LEVEL_PROPERTY_SEED_COUNT;

    constexpr std::uint64_t kCampaignSeedCount =
        ANTWIKA_CAMPAIGN_SEED_COUNT;

    static_assert(
        kPropertySeedCount <= kSeedCount,
        "property cases read their levels off the soak's pool");

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

    void expectSingleSimplePath(const Level &level)
    {
        ASSERT_GE(level.path.size(), 2U);
        EXPECT_EQ(level.at(level.path.front()), Tile::Start);
        EXPECT_EQ(level.at(level.path.back()), Tile::End);

        std::set<std::pair<std::uint32_t, std::uint32_t>> seen;
        for (const Cell &cell : level.path)
        {
            EXPECT_TRUE(seen.emplace(cell.x, cell.y).second)
                << cell.x << ' ' << cell.y;
            EXPECT_LT(cell.x, level.width);
            EXPECT_LT(cell.y, level.height);
        }

        for (std::size_t i = 1; i < level.path.size(); ++i)
        {
            EXPECT_TRUE(adjacent(level.path[i - 1], level.path[i])) << i;
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

                EXPECT_LE(std::popcount(openSides(tile)), 2);
            }
        }

        EXPECT_EQ(starts, 1U);
        EXPECT_EQ(ends, 1U);
        EXPECT_EQ(occupied, level.path.size());
    }

    TEST(LevelGeneratorTest, GenerateLevel_GivesOneSimplePath)
    {
        for (std::uint64_t seed = 0; seed < kSeedCount; ++seed)
        {
            SCOPED_TRACE("seed " + std::to_string(seed));
            expectSingleSimplePath(sweptLevels()[seed]);
        }
    }

    TEST(LevelGeneratorTest, GenerateLevel_MatchesEveryOpenSide)
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

    TEST(LevelGeneratorTest, GenerateLevel_MatchesTheRecordedLevelForItsSeed)
    {
        const Level level = generateLevel({.seed = 7});

        EXPECT_EQ(level.width, 12U);
        EXPECT_EQ(level.height, 8U);
        EXPECT_EQ(
            level.path,
            (std::vector<Cell>{
                {0, 3}, {1, 3}, {2, 3}, {2, 4}, {2, 5}, {3, 5},
                {4, 5}, {5, 5}, {6, 5}, {7, 5}, {8, 5}, {9, 5},
                {10, 5}, {10, 4}, {10, 3}, {10, 2}, {10, 1},
                {11, 1}}));
    }

    TEST(LevelGeneratorTest, GenerateLevel_VariesBySeed)
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

    TEST(LevelGeneratorTest, GenerateLevel_StillPathsASmallGrid)
    {
        const Level level =
            generateLevel({.width = 3, .height = 1, .seed = 1});
        expectSingleSimplePath(level);
        EXPECT_EQ(level.path.size(), 3U);
    }

    TEST(LevelGeneratorTest, GenerateLevel_NeverWallsTheEndColumn)
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

    TEST(LevelGeneratorTest, GenerateLevel_ReproducesEveryShippedLevel)
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

    TEST(LevelGeneratorTest, GenerateLevel_RefusesATooNarrowGrid)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel({.width = 2})),
            LevelError);
    }

    TEST(LevelGeneratorTest, GenerateLevel_RefusesAZeroHeightGrid)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel({.height = 0})),
            LevelError);
    }

    TEST(LevelGeneratorTest, GenerateLevel_RefusesAWalledColumn)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel({.wallSpacing = 1})),
            LevelError);
    }

    TEST(LevelGeneratorTest, GenerateLevel_RefusesAZeroBudget)
    {
        EXPECT_THROW(
            static_cast<void>(
                generateLevel({.initialSolverSteps = 0})),
            LevelError);
    }

    TEST(LevelGeneratorTest, GenerateLevel_RefusesABudgetAboveTheCap)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel(
                {.initialSolverSteps = 2, .maxSolverSteps = 1})),
            LevelError);
    }

    TEST(LevelGeneratorTest, GenerateLevel_ReportsAnExhaustedBudget)
    {
        EXPECT_THROW(
            static_cast<void>(generateLevel(
                {.initialSolverSteps = 1,
                 .maxSolverSteps = 1,
                 .maxAttempts = 2})),
            LevelError);
    }

    TEST(LevelGeneratorTest, GenerateLevel_BoundsAnEscalatingBudget)
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
}
