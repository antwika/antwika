#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

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

    // A wave whose second mob is further off than any test runs.
    // A case wanting exactly one walker therefore gets one.
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

    TEST(BattleTest, ATowerCannotStandOnThePath)
    {
        Battle battle(straightLevel(8), BattleConfig{}, nothing());
        EXPECT_FALSE(battle.placeTower({.x = 3, .y = 0}));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(BattleTest, ATowerCannotStandOutsideTheGrid)
    {
        Battle battle(straightLevel(8), BattleConfig{}, nothing());
        EXPECT_FALSE(battle.placeTower({.x = 8, .y = 0}));
        EXPECT_FALSE(battle.placeTower({.x = 0, .y = 3}));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(BattleTest, TwoTowersCannotShareACell)
    {
        Battle battle(straightLevel(8), BattleConfig{}, nothing());
        EXPECT_TRUE(battle.placeTower({.x = 3, .y = 1}));
        EXPECT_FALSE(battle.placeTower({.x = 3, .y = 1}));
        EXPECT_TRUE(battle.placeTower({.x = 4, .y = 1}));
        ASSERT_EQ(battle.towers().size(), 2U);
        EXPECT_EQ(battle.towers()[0].id, 0U);
        EXPECT_EQ(battle.towers()[1].id, 1U);
    }

    TEST(BattleTest, AWaveReleasesOneMobEverySpawnPeriod)
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

        // Three ticks of quiet, then the next one.
        stepMany(battle, 3);
        EXPECT_EQ(battle.mobs().size(), 1U);
        battle.step();
        EXPECT_EQ(battle.mobs().size(), 2U);

        stepMany(battle, 4);
        EXPECT_EQ(battle.mobs().size(), 3U);
        EXPECT_EQ(battle.wavesReleased(), 1U);
        EXPECT_EQ(battle.waveCount(), 1U);
    }

    // The one axis that decides how long a mob spends inside a reach.
    TEST(BattleTest, EachKindCrossesACellEveryTicksPerCellTicks)
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
                << "kind " << static_cast<int>(kind);

            stepMany(battle, static_cast<int>(pace));
            ASSERT_EQ(battle.mobs().size(), 1U);
            EXPECT_EQ(battle.mobs()[0].pathIndex, 2U);
        }
    }

    TEST(BattleTest, AMobReachingTheEndIsReportedAsALeak)
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

    TEST(BattleTest, ATowerInRangeKillsAMobAndPaysItsReward)
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

    TEST(BattleTest, ATowerOutOfRangeNeverFires)
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

    // Armour is taken off every hit and the result never heals.
    // So a gun that does not out-damage it is simply wasted.
    TEST(BattleTest, ArmourComesOffEveryHitAndAWeakGunIsWasted)
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

        // One point of damage against one point of armour is nothing.
        EXPECT_EQ(fight(1), 0U);
        EXPECT_EQ(fight(3), profileOf(MobKind::Shielded).reward);
    }

    // Kinds walk at different paces.
    // So two mobs can share a cell and a Runner can pass a Brute.
    // Which is why the target is chosen by a written-out ordering.
    TEST(BattleTest, TheFurthestAlongIsShotAndATieGoesToTheOlderMob)
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

    TEST(BattleTest, TheNextWaveFollowsAfterTheGap)
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

    // A caller can describe a wave with nothing in it.
    // Stepping over it is what stops one from stalling a campaign.
    TEST(BattleTest, AnEmptyWaveIsSteppedOverRatherThanStalling)
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

    // A level with no path gives a mob no cell to stand on.
    // Its waves pass without releasing anything.
    // So the level still clears rather than stalling a campaign.
    TEST(BattleTest, ALevelWithNoPathReleasesNothingAndStillClears)
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

    TEST(BattleTest, ALevelIsClearedOnlyOnceNothingIsLeft)
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

    TEST(BattleTest, TheSameInputsGiveTheSameBattle)
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

        const std::uint64_t firstScore = run(first);
        EXPECT_EQ(firstScore, run(second));
        EXPECT_EQ(first.mobs().size(), second.mobs().size());
        EXPECT_GT(firstScore, 0U);
    }

    TEST(BattleTest, TheLevelIsHandedBackForDrawing)
    {
        Battle battle(straightLevel(5), BattleConfig{}, nothing());
        EXPECT_EQ(battle.level().width, 5U);
        EXPECT_EQ(battle.level().path.size(), 5U);
        EXPECT_EQ(battle.settings().towerDamage, 1);
    }
} // namespace
