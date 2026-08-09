#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/LevelTile.hpp"
#include "antwika/tower_defence/MobKind.hpp"
#include "antwika/tower_defence/Wave.hpp"

using antwika::tower_defence::Battle;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::Cell;
using antwika::tower_defence::Level;
using antwika::tower_defence::Mob;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::profileOf;
using antwika::tower_defence::StepOutcome;
using antwika::tower_defence::Tile;
using antwika::tower_defence::WaveRelease;

namespace
{
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

    std::vector<WaveRelease> oneWaveOf(
        const MobKind kind,
        const std::uint32_t count,
        const std::uint64_t period = 1000)
    {
        return {WaveRelease{
            .order = std::vector<MobKind>(count, kind),
            .spawnPeriodTicks = period,
            .gapTicks = 0}};
    }

    std::vector<WaveRelease> nothing()
    {
        return oneWaveOf(MobKind::Grunt, 2, 1000);
    }

    std::uint64_t stepMany(Battle &battle, const int times)
    {
        std::uint64_t earned = 0;
        for (int step = 0; step < times; ++step)
        {
            earned += battle.step().reward;
        }
        return earned;
    }

    TEST(BattleTest, PlaceTower_RefusesThePath)
    {
        Battle battle(straightLevel(8), BattleConfig{}, nothing());
        EXPECT_FALSE(battle.placeTower({.x = 3, .y = 0}));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(BattleTest, PlaceTower_RefusesOutsideTheGrid)
    {
        Battle battle(straightLevel(8), BattleConfig{}, nothing());
        EXPECT_FALSE(battle.placeTower({.x = 8, .y = 0}));
        EXPECT_FALSE(battle.placeTower({.x = 0, .y = 3}));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(BattleTest, PlaceTower_RefusesASharedCell)
    {
        Battle battle(straightLevel(8), BattleConfig{}, nothing());
        EXPECT_TRUE(battle.placeTower({.x = 3, .y = 1}));
        EXPECT_FALSE(battle.placeTower({.x = 3, .y = 1}));
        EXPECT_TRUE(battle.placeTower({.x = 4, .y = 1}));
        ASSERT_EQ(battle.towers().size(), 2U);
        EXPECT_EQ(battle.towers()[0].id, 0U);
        EXPECT_EQ(battle.towers()[1].id, 1U);
    }

    TEST(BattleTest, Step_ReleasesOneMobPerSpawnPeriod)
    {
        Battle battle(
            straightLevel(12),
            BattleConfig{},
            oneWaveOf(MobKind::Grunt, 3, 3));

        battle.step();
        ASSERT_EQ(battle.mobs().size(), 1U);
        EXPECT_EQ(battle.mobs()[0].kind, MobKind::Grunt);
        EXPECT_EQ(
            battle.mobs()[0].health, profileOf(MobKind::Grunt).health);
        EXPECT_EQ(battle.ticks(), 1U);

        stepMany(battle, 3);
        EXPECT_EQ(battle.mobs().size(), 1U);
        battle.step();
        EXPECT_EQ(battle.mobs().size(), 2U);

        stepMany(battle, 4);
        EXPECT_EQ(battle.mobs().size(), 3U);
        EXPECT_EQ(battle.wavesReleased(), 1U);
        EXPECT_EQ(battle.waveCount(), 1U);
    }

    TEST(BattleTest, Step_MovesEachKindAtItsOwnRate)
    {
        for (const MobKind kind : antwika::tower_defence::kAllMobKinds)
        {
            const std::uint32_t pace = profileOf(kind).ticksPerCell;
            Battle battle(
                straightLevel(20), BattleConfig{}, oneWaveOf(kind, 1));

            battle.step();
            ASSERT_EQ(battle.mobs().size(), 1U);
            EXPECT_EQ(battle.mobs()[0].pathIndex, 0U);

            stepMany(battle, static_cast<int>(pace));
            ASSERT_EQ(battle.mobs().size(), 1U);
            EXPECT_EQ(battle.mobs()[0].pathIndex, 1U)
                << static_cast<int>(kind);

            stepMany(battle, static_cast<int>(pace));
            ASSERT_EQ(battle.mobs().size(), 1U);
            EXPECT_EQ(battle.mobs()[0].pathIndex, 2U);
        }
    }

    TEST(BattleTest, Step_ReportsAMobReachingTheEndAsALeak)
    {
        Battle battle(
            straightLevel(4),
            BattleConfig{},
            oneWaveOf(MobKind::Runner, 1));

        std::uint32_t leaks = 0;
        for (int step = 0; step < 8; ++step)
        {
            leaks += battle.step().leaks;
        }
        EXPECT_EQ(leaks, 1U);
        EXPECT_TRUE(battle.mobs().empty());
        EXPECT_TRUE(battle.cleared());
    }

    TEST(BattleTest, Step_KillsInRangeAndPaysTheReward)
    {
        Battle battle(
            straightLevel(8),
            BattleConfig{.towerRangeSquared = 4, .towerDamage = 2},
            oneWaveOf(MobKind::Runner, 1));
        ASSERT_TRUE(battle.placeTower({.x = 0, .y = 1}));

        const std::uint64_t earned = stepMany(battle, 3);
        EXPECT_TRUE(battle.mobs().empty());
        EXPECT_EQ(earned, profileOf(MobKind::Runner).reward);
    }

    TEST(BattleTest, Step_NeverFiresOutOfRange)
    {
        Battle battle(
            straightLevel(8),
            BattleConfig{.towerRangeSquared = 1, .towerDamage = 9},
            oneWaveOf(MobKind::Grunt, 1));
        ASSERT_TRUE(battle.placeTower({.x = 6, .y = 2}));

        EXPECT_EQ(stepMany(battle, 3), 0U);
        ASSERT_EQ(battle.mobs().size(), 1U);
        EXPECT_EQ(
            battle.mobs()[0].health, profileOf(MobKind::Grunt).health);
    }

    TEST(BattleTest, Step_TakesArmourOffEveryHit)
    {
        const auto fight = [](const std::int32_t damage)
        {
            Battle battle(
                straightLevel(8),
                BattleConfig{
                    .towerRangeSquared = 64, .towerDamage = damage},
                oneWaveOf(MobKind::Shielded, 1));
            static_cast<void>(battle.placeTower({.x = 0, .y = 1}));
            return stepMany(battle, 10);
        };

        EXPECT_EQ(fight(1), 0U);
        EXPECT_EQ(fight(3), profileOf(MobKind::Shielded).reward);
    }

    TEST(BattleTest, Step_ShootsTheFurthestAlongThenTheOlder)
    {
        Battle battle(
            straightLevel(14),
            BattleConfig{.towerRangeSquared = 25, .towerDamage = 1},
            {WaveRelease{
                .order = {MobKind::Brute, MobKind::Runner},
                .spawnPeriodTicks = 1,
                .gapTicks = 0}});
        ASSERT_TRUE(battle.placeTower({.x = 1, .y = 1}));

        bool sawATieShootTheOlder = false;
        bool sawAnOvertakeShootTheNewer = false;

        for (int step = 0; step < 14; ++step)
        {
            const std::vector<Mob> before = battle.mobs();
            battle.step();
            const std::vector<Mob> &after = battle.mobs();

            if (before.size() != 2 || after.size() != 2
                || before[0].id != after[0].id
                || before[1].id != after[1].id)
            {
                continue;
            }

            const bool olderHit = after[0].health < before[0].health;
            const bool newerHit = after[1].health < before[1].health;

            if (after[0].pathIndex == after[1].pathIndex && olderHit
                && !newerHit)
            {
                sawATieShootTheOlder = true;
            }
            if (after[1].pathIndex > after[0].pathIndex && newerHit
                && !olderHit)
            {
                sawAnOvertakeShootTheNewer = true;
            }
        }

        EXPECT_TRUE(sawATieShootTheOlder);
        EXPECT_TRUE(sawAnOvertakeShootTheNewer);
    }

    TEST(BattleTest, Step_ReleasesTheNextWaveAfterTheGap)
    {
        Battle battle(
            straightLevel(12),
            BattleConfig{},
            {WaveRelease{
                 .order = {MobKind::Grunt},
                 .spawnPeriodTicks = 1,
                 .gapTicks = 3},
             WaveRelease{
                 .order = {MobKind::Runner},
                 .spawnPeriodTicks = 1,
                 .gapTicks = 0}});

        EXPECT_EQ(battle.waveCount(), 2U);

        battle.step();
        EXPECT_EQ(battle.wavesReleased(), 1U);
        EXPECT_EQ(battle.mobs().size(), 1U);

        stepMany(battle, 3);
        EXPECT_EQ(battle.wavesReleased(), 1U);
        EXPECT_EQ(battle.mobs().size(), 1U);

        battle.step();
        EXPECT_EQ(battle.wavesReleased(), 2U);
        ASSERT_EQ(battle.mobs().size(), 2U);
        EXPECT_EQ(battle.mobs()[1].kind, MobKind::Runner);
    }

    TEST(BattleTest, Step_StepsOverAnEmptyWave)
    {
        Battle battle(
            straightLevel(8),
            BattleConfig{},
            {WaveRelease{
                 .order = {}, .spawnPeriodTicks = 1, .gapTicks = 0},
             WaveRelease{
                 .order = {MobKind::Grunt},
                 .spawnPeriodTicks = 1,
                 .gapTicks = 0}});

        battle.step();
        EXPECT_EQ(battle.wavesReleased(), 1U);
        EXPECT_TRUE(battle.mobs().empty());

        battle.step();
        EXPECT_EQ(battle.wavesReleased(), 2U);
        EXPECT_EQ(battle.mobs().size(), 1U);
    }

    TEST(BattleTest, Step_ClearsALevelWithNoPath)
    {
        Level pathless = straightLevel(8);
        pathless.path.clear();

        Battle battle(
            pathless, BattleConfig{}, oneWaveOf(MobKind::Grunt, 4, 1));

        const StepOutcome outcome = battle.step();
        EXPECT_EQ(outcome.leaks, 0U);
        EXPECT_EQ(outcome.reward, 0U);
        EXPECT_TRUE(battle.mobs().empty());
        EXPECT_TRUE(battle.cleared());
    }

    TEST(BattleTest, Cleared_IsTrueOnlyOnceNothingIsLeft)
    {
        Battle battle(
            straightLevel(6),
            BattleConfig{},
            oneWaveOf(MobKind::Runner, 1));

        EXPECT_FALSE(battle.cleared());
        battle.step();
        EXPECT_FALSE(battle.cleared());
        EXPECT_EQ(battle.wavesReleased(), 1U);

        stepMany(battle, 10);
        EXPECT_TRUE(battle.cleared());
    }

    TEST(BattleTest, Step_IsIdenticalForTheSameInputs)
    {
        const auto run = [](Battle &battle)
        {
            static_cast<void>(battle.placeTower({.x = 2, .y = 1}));
            static_cast<void>(battle.placeTower({.x = 5, .y = 2}));
            return stepMany(battle, 40);
        };
        Battle first(
            straightLevel(9),
            BattleConfig{},
            oneWaveOf(MobKind::Grunt, 6, 2));
        Battle second(
            straightLevel(9),
            BattleConfig{},
            oneWaveOf(MobKind::Grunt, 6, 2));

        EXPECT_EQ(run(first), 10U);
        EXPECT_TRUE(first.mobs().empty());

        EXPECT_EQ(run(second), 10U);
        EXPECT_TRUE(second.mobs().empty());
    }

    TEST(BattleTest, Level_IsHandedBackForDrawing)
    {
        Battle battle(straightLevel(5), BattleConfig{}, nothing());
        EXPECT_EQ(battle.level().width, 5U);
        EXPECT_EQ(battle.level().path.size(), 5U);
        EXPECT_EQ(battle.settings().towerDamage, 1);
    }
}
