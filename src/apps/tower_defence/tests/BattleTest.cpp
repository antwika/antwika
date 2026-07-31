#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/LevelTile.hpp"

using antwika::tower_defence::Battle;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::Cell;
using antwika::tower_defence::Level;
using antwika::tower_defence::Tile;

namespace
{
    // A hand-built level, so a battle test never runs the solver.
    // One straight run along row 0, west to east.
    Level straightLevel(const std::uint32_t width)
    {
        Level level{
            .width = width,
            .height = 3,
            .tiles = std::vector<Tile>(
                static_cast<std::size_t>(width) * 3, Tile::Empty),
            .path = {}};
        for (std::uint32_t x = 0; x < width; ++x)
        {
            const Cell cell{.x = x, .y = 0};
            level.path.push_back(cell);
            level.tiles[x] = Tile::EastWest;
        }
        level.tiles[0] = Tile::Start;
        level.tiles[width - 1] = Tile::End;
        return level;
    }

    BattleConfig quietConfig()
    {
        // A spawn period longer than any test runs.
        // A test wanting exactly one mob therefore gets exactly one.
        return BattleConfig{.spawnPeriodTicks = 1000};
    }

    TEST(BattleTest, ATowerCannotStandOnThePath)
    {
        Battle battle(straightLevel(8), quietConfig());
        EXPECT_FALSE(battle.placeTower({.x = 3, .y = 0}));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(BattleTest, ATowerCannotStandOutsideTheGrid)
    {
        Battle battle(straightLevel(8), quietConfig());
        EXPECT_FALSE(battle.placeTower({.x = 8, .y = 0}));
        EXPECT_FALSE(battle.placeTower({.x = 0, .y = 3}));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(BattleTest, TwoTowersCannotShareACell)
    {
        Battle battle(straightLevel(8), quietConfig());
        EXPECT_TRUE(battle.placeTower({.x = 3, .y = 1}));
        EXPECT_FALSE(battle.placeTower({.x = 3, .y = 1}));
        EXPECT_TRUE(battle.placeTower({.x = 4, .y = 1}));
        ASSERT_EQ(battle.towers().size(), 2U);
        EXPECT_EQ(battle.towers()[0].id, 0U);
        EXPECT_EQ(battle.towers()[1].id, 1U);
    }

    TEST(BattleTest, AMobSpawnsOnTheSpawnPeriodAndWalksOneCellATick)
    {
        Battle battle(
            straightLevel(8), BattleConfig{.spawnPeriodTicks = 4});
        battle.step();
        ASSERT_EQ(battle.mobs().size(), 1U);
        EXPECT_EQ(battle.mobs()[0].pathIndex, 0U);
        EXPECT_EQ(battle.ticks(), 1U);

        battle.step();
        ASSERT_EQ(battle.mobs().size(), 1U);
        EXPECT_EQ(battle.mobs()[0].pathIndex, 1U);

        battle.step();
        battle.step();
        battle.step();
        EXPECT_EQ(battle.mobs().size(), 2U);
    }

    TEST(BattleTest, AMobReachingTheEndLeaksAndCostsScore)
    {
        Battle battle(
            straightLevel(4),
            BattleConfig{
                .spawnPeriodTicks = 1000,
                .killScore = 10,
                .leakPenalty = 25});
        for (int i = 0; i < 4; ++i)
        {
            battle.step();
        }
        EXPECT_EQ(battle.leaks(), 1U);
        EXPECT_TRUE(battle.mobs().empty());

        // The score floors at zero rather than wrapping.
        EXPECT_EQ(battle.score(), 0U);
    }

    TEST(BattleTest, ATowerInRangeKillsAMobAndScores)
    {
        Battle battle(
            straightLevel(8),
            BattleConfig{
                .spawnPeriodTicks = 1000,
                .mobHealth = 2,
                .towerRangeSquared = 4,
                .towerDamage = 1,
                .killScore = 10});
        ASSERT_TRUE(battle.placeTower({.x = 0, .y = 1}));

        battle.step();
        ASSERT_EQ(battle.mobs().size(), 1U);
        EXPECT_EQ(battle.mobs()[0].health, 1);
        EXPECT_EQ(battle.score(), 0U);

        battle.step();
        EXPECT_TRUE(battle.mobs().empty());
        EXPECT_EQ(battle.score(), 10U);
    }

    TEST(BattleTest, ATowerOutOfRangeNeverFires)
    {
        Battle battle(
            straightLevel(8),
            BattleConfig{
                .spawnPeriodTicks = 1000,
                .mobHealth = 2,
                .towerRangeSquared = 1});
        ASSERT_TRUE(battle.placeTower({.x = 6, .y = 2}));
        battle.step();
        battle.step();
        ASSERT_EQ(battle.mobs().size(), 1U);
        EXPECT_EQ(battle.mobs()[0].health, 2);
        EXPECT_EQ(battle.score(), 0U);
    }

    TEST(BattleTest, ATowerShootsTheMobFurthestAlongTheRun)
    {
        Battle battle(
            straightLevel(10),
            BattleConfig{
                .spawnPeriodTicks = 1,
                .mobHealth = 9,
                .towerRangeSquared = 9,
                .towerDamage = 1});
        ASSERT_TRUE(battle.placeTower({.x = 1, .y = 1}));
        battle.step();
        battle.step();
        battle.step();

        // Three mobs are alive and all three are inside the reach.
        // Only the oldest one, at the front, has been shot at all.
        ASSERT_EQ(battle.mobs().size(), 3U);
        EXPECT_EQ(battle.mobs()[0].pathIndex, 2U);
        EXPECT_EQ(battle.mobs()[0].health, 6);
        EXPECT_EQ(battle.mobs()[1].health, 9);
        EXPECT_EQ(battle.mobs()[2].health, 9);
    }

    // Battle::fire() shoots the first mob in reach.
    // That is only right if the mobs are in descending path order.
    // This is what pins that ordering.
    // Spawn order is ascending and every mob walks one cell a tick.
    // Leaks are removed in place, so nothing ever reorders.
    TEST(BattleTest, MobsStayInStrictlyDescendingPathOrder)
    {
        Battle battle(
            straightLevel(12),
            BattleConfig{.spawnPeriodTicks = 2, .mobHealth = 100});
        for (int i = 0; i < 20; ++i)
        {
            battle.step();
            const auto &mobs = battle.mobs();
            for (std::size_t j = 1; j < mobs.size(); ++j)
            {
                EXPECT_GT(mobs[j - 1].pathIndex, mobs[j].pathIndex)
                    << "tick " << i << ", mob " << j;
            }
        }
        EXPECT_GT(battle.mobs().size(), 1U);
    }

    TEST(BattleTest, TheSameInputsGiveTheSameBattle)
    {
        const auto run = [](Battle &battle)
        {
            static_cast<void>(battle.placeTower({.x = 2, .y = 1}));
            static_cast<void>(battle.placeTower({.x = 5, .y = 2}));
            for (int i = 0; i < 40; ++i)
            {
                battle.step();
            }
        };
        Battle first(straightLevel(9), BattleConfig{});
        Battle second(straightLevel(9), BattleConfig{});
        run(first);
        run(second);
        EXPECT_EQ(first.score(), second.score());
        EXPECT_EQ(first.leaks(), second.leaks());
        EXPECT_EQ(first.mobs().size(), second.mobs().size());
        EXPECT_GT(first.score(), 0U);
    }

    TEST(BattleTest, TheLevelIsHandedBackForDrawing)
    {
        Battle battle(straightLevel(5), quietConfig());
        EXPECT_EQ(battle.level().width, 5U);
        EXPECT_EQ(battle.level().path.size(), 5U);
    }
} // namespace
