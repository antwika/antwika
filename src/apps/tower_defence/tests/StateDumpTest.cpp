#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/StateDump.hpp"
#include "antwika/tower_defence/TowerDefenceSnapshotStore.hpp"
#include "antwika/tower_defence/Wave.hpp"

using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleMemory;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::CampaignConfig;
using antwika::tower_defence::CampaignMemory;
using antwika::tower_defence::CampaignPhase;
using antwika::tower_defence::Cell;
using antwika::tower_defence::LevelPlan;
using antwika::tower_defence::Mob;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::StateDump;
using antwika::tower_defence::StateDumpError;
using antwika::tower_defence::Tower;
using antwika::tower_defence::TowerDefenceSnapshotStore;
using antwika::tower_defence::Wave;
using antwika::tower_defence::WaveEntry;

namespace
{
    CampaignConfig tinyCampaign()
    {
        return CampaignConfig{
            .seed = 3,
            .lives = 20,
            .levels = {
                LevelPlan{
                    .level =
                        {.width = 7, .height = 5, .wallSpacing = 3},
                    .battle = BattleConfig{},
                    .waves = {Wave{
                        .entries = {WaveEntry{MobKind::Grunt, 2}},
                        .spawnPeriodTicks = 3,
                        .gapTicks = 0}}},
                LevelPlan{
                    .level =
                        {.width = 8, .height = 5, .wallSpacing = 3},
                    .battle = BattleConfig{},
                    .waves = {Wave{
                        .entries = {WaveEntry{MobKind::Runner, 1}},
                        .spawnPeriodTicks = 3,
                        .gapTicks = 0}}}}};
    }

    StateDump fullDump()
    {
        return StateDump{
            .campaign =
                CampaignMemory{
                    .level = 1,
                    .score = 44,
                    .lives = 9,
                    .ticks = 123,
                    .phase = CampaignPhase::Lost,
                    .battle =
                        BattleMemory{
                            .waveIndex = 1,
                            .spawnedInWave = 2,
                            .ticksUntilRelease = 4,
                            .tickCount = 77,
                            .nextMobId = 6,
                            .nextTowerId = 3,
                            .mobs =
                                {Mob{
                                     .id = 4,
                                     .kind = MobKind::Grunt,
                                     .pathIndex = 2,
                                     .health = 5,
                                     .ticksUntilStep = 1},
                                 Mob{
                                     .id = 5,
                                     .kind = MobKind::Runner,
                                     .pathIndex = 3,
                                     .health = 4,
                                     .ticksUntilStep = 0},
                                 Mob{
                                     .id = 6,
                                     .kind = MobKind::Brute,
                                     .pathIndex = 1,
                                     .health = 18,
                                     .ticksUntilStep = 2},
                                 Mob{
                                     .id = 7,
                                     .kind = MobKind::Shielded,
                                     .pathIndex = 4,
                                     .health = 8,
                                     .ticksUntilStep = 1}},
                            .towers =
                                {Tower{
                                    .id = 2,
                                    .cell = {.x = 3, .y = 1}}}}},
            .bestScore = 250};
    }

    void stepUntilAMobWalks(Campaign &campaign)
    {
        while (campaign.battle().mobs().empty())
        {
            campaign.step();
        }
    }
}

TEST(StateDumpTest, StateDumpFromJson_RoundTripsTheDump)
{
    const StateDump dump = fullDump();

    const auto decoded =
        antwika::tower_defence::stateDumpFromJson(
            antwika::tower_defence::stateDumpToJson(dump));

    EXPECT_EQ(decoded, dump);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryMobField)
{
    const Mob base = fullDump().campaign.battle.mobs[0];

    Mob named = base;
    named.id = 99;
    EXPECT_NE(base, named);

    Mob kinded = base;
    kinded.kind = MobKind::Shielded;
    EXPECT_NE(base, kinded);

    Mob walked = base;
    walked.pathIndex = 9;
    EXPECT_NE(base, walked);

    Mob hurt = base;
    hurt.health = 1;
    EXPECT_NE(base, hurt);

    Mob paced = base;
    paced.ticksUntilStep = 7;
    EXPECT_NE(base, paced);

    EXPECT_EQ(base, Mob{base});
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryTowerField)
{
    const Tower base = fullDump().campaign.battle.towers[0];

    Tower named = base;
    named.id = 99;
    EXPECT_NE(base, named);

    Tower moved = base;
    moved.cell.x = 6;
    EXPECT_NE(base, moved);

    EXPECT_EQ(base, Tower{base});
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryBattleMemoryField)
{
    const BattleMemory base = fullDump().campaign.battle;

    BattleMemory waved = base;
    waved.waveIndex = 9;
    EXPECT_NE(base, waved);

    BattleMemory spawned = base;
    spawned.spawnedInWave = 9;
    EXPECT_NE(base, spawned);

    BattleMemory held = base;
    held.ticksUntilRelease = 9;
    EXPECT_NE(base, held);

    BattleMemory ticked = base;
    ticked.tickCount = 9;
    EXPECT_NE(base, ticked);

    BattleMemory mobNamed = base;
    mobNamed.nextMobId = 9;
    EXPECT_NE(base, mobNamed);

    BattleMemory towerNamed = base;
    towerNamed.nextTowerId = 9;
    EXPECT_NE(base, towerNamed);

    BattleMemory walked = base;
    walked.mobs.clear();
    EXPECT_NE(base, walked);

    BattleMemory gunned = base;
    gunned.towers.clear();
    EXPECT_NE(base, gunned);

    EXPECT_EQ(base, BattleMemory{base});
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryCampaignField)
{
    const CampaignMemory base = fullDump().campaign;

    CampaignMemory levelled = base;
    levelled.level = 9;
    EXPECT_NE(base, levelled);

    CampaignMemory scored = base;
    scored.score = 9;
    EXPECT_NE(base, scored);

    CampaignMemory lively = base;
    lively.lives = 3;
    EXPECT_NE(base, lively);

    CampaignMemory ticked = base;
    ticked.ticks = 9;
    EXPECT_NE(base, ticked);

    CampaignMemory phased = base;
    phased.phase = CampaignPhase::Won;
    EXPECT_NE(base, phased);

    CampaignMemory fought = base;
    fought.battle.mobs.clear();
    EXPECT_NE(base, fought);

    EXPECT_EQ(base, CampaignMemory{base});
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryStateDumpField)
{
    const StateDump base = fullDump();

    StateDump run = base;
    run.campaign.score = 9;
    EXPECT_NE(base, run);

    StateDump bested = base;
    bested.bestScore = 9;
    EXPECT_NE(base, bested);

    EXPECT_EQ(base, StateDump{base});
}

TEST(StateDumpTest, StateDumpFromJson_KeepsEveryPhaseName)
{
    for (const auto phase :
         {CampaignPhase::Fighting,
          CampaignPhase::Won,
          CampaignPhase::Lost})
    {
        StateDump dump = fullDump();
        dump.campaign.phase = phase;

        const auto decoded =
            antwika::tower_defence::stateDumpFromJson(
                antwika::tower_defence::stateDumpToJson(dump));

        EXPECT_EQ(decoded.campaign.phase, phase);
    }
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAnUnknownMobKind)
{
    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state["battle"]["mobs"][0]["kind"] = "dragon";

    EXPECT_THROW(
        (void)antwika::tower_defence::stateDumpFromJson(state),
        StateDumpError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAnUnknownPhase)
{
    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state["phase"] = "resting";

    EXPECT_THROW(
        (void)antwika::tower_defence::stateDumpFromJson(state),
        StateDumpError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAMissingMember)
{
    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state.erase("lives");

    EXPECT_THROW(
        (void)antwika::tower_defence::stateDumpFromJson(state),
        StateDumpError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesADumpWithALevel)
{
    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state["levelData"] = nlohmann::json::object();

    EXPECT_THROW(
        (void)antwika::tower_defence::stateDumpFromJson(state),
        StateDumpError);
}

TEST(StateDumpTest, StandardStateDumpMigrations_HaveNoSteps)
{
    const auto chain =
        antwika::tower_defence::standardStateDumpMigrations();

    nlohmann::json document;
    document["version"] = antwika::tower_defence::kStateDumpVersion;
    const auto before = document;
    chain.migrate(document);

    EXPECT_EQ(document, before);
}

TEST(StateDumpTest, Restore_RebuildsARememberedCampaign)
{
    Campaign played(tinyCampaign());
    stepUntilAMobWalks(played);
    ASSERT_TRUE(played.placeTower(Cell{.x = 0, .y = 0})
                || played.placeTower(Cell{.x = 1, .y = 0})
                || played.placeTower(Cell{.x = 0, .y = 1}));

    const CampaignMemory memory = played.remember();

    Campaign fresh(tinyCampaign());
    ASSERT_TRUE(fresh.restore(memory));

    EXPECT_EQ(fresh.remember(), memory);
    EXPECT_EQ(fresh.score(), played.score());
    EXPECT_EQ(fresh.lives(), played.lives());
    EXPECT_EQ(fresh.ticks(), played.ticks());
    EXPECT_EQ(fresh.battle().mobs(), played.battle().mobs());
    EXPECT_EQ(fresh.battle().towers(), played.battle().towers());
}

TEST(StateDumpTest, Restore_RegeneratesTheLevelFromTheSeed)
{
    Campaign played(tinyCampaign());
    while (played.levelIndex() == 0)
    {
        played.step();
        ASSERT_LT(played.ticks(), 500U);
    }

    const CampaignMemory memory = played.remember();
    ASSERT_EQ(memory.level, 1U);

    Campaign fresh(tinyCampaign());
    ASSERT_TRUE(fresh.restore(memory));

    EXPECT_EQ(
        fresh.battle().level().path, played.battle().level().path);
    EXPECT_EQ(
        fresh.battle().level().tiles, played.battle().level().tiles);
}

TEST(StateDumpTest, Restore_RefusesAMobPastThePath)
{
    Campaign campaign(tinyCampaign());
    const std::uint64_t before = campaign.ticks();

    CampaignMemory memory = campaign.remember();
    memory.ticks = 42;
    memory.battle.mobs.push_back(Mob{
        .id = 0,
        .kind = MobKind::Grunt,
        .pathIndex = 100000,
        .health = 1,
        .ticksUntilStep = 0});

    EXPECT_FALSE(campaign.restore(memory));
    EXPECT_EQ(campaign.ticks(), before);
}

TEST(StateDumpTest, Restore_RefusesAnUnreachableLevelIndex)
{
    Campaign campaign(tinyCampaign());

    CampaignMemory memory = campaign.remember();
    memory.level = tinyCampaign().levels.size() + 1;

    EXPECT_FALSE(campaign.restore(memory));
}

TEST(StateDumpTest, Restore_RebuildsAWonCampaign)
{
    Campaign campaign(tinyCampaign());

    CampaignMemory memory = campaign.remember();
    memory.level = tinyCampaign().levels.size();
    memory.phase = CampaignPhase::Won;
    memory.score = 99;
    memory.battle = BattleMemory{
        .waveIndex = 1,
        .spawnedInWave = 0,
        .ticksUntilRelease = 0,
        .tickCount = 200,
        .nextMobId = 3,
        .nextTowerId = 1,
        .mobs = {},
        .towers = {Tower{.id = 0, .cell = {.x = 2, .y = 2}}}};

    ASSERT_TRUE(campaign.restore(memory));

    EXPECT_EQ(campaign.phase(), CampaignPhase::Won);
    EXPECT_EQ(campaign.score(), 99U);
    EXPECT_TRUE(campaign.battle().level().path.empty());
    EXPECT_EQ(campaign.battle().towers().size(), 1U);
}

TEST(StateDumpTest, Load_ComesBackToTheWholeDumpedState)
{
    const antwika::testing::ScratchFile file(
        "antwika_tower_defence_store_round_trip.json");
    const auto path = file.path().string();

    Campaign played(tinyCampaign());
    stepUntilAMobWalks(played);
    std::uint64_t playedBest = 7;
    TowerDefenceSnapshotStore store(played, playedBest);

    store.dump(path, {"> dump_state"});

    Campaign fresh(tinyCampaign());
    std::uint64_t freshBest = 0;
    TowerDefenceSnapshotStore reader(fresh, freshBest);

    const auto history = reader.load(path);

    EXPECT_EQ(history, (std::vector<std::string>{"> dump_state"}));
    EXPECT_EQ(fresh.remember(), played.remember());
    EXPECT_EQ(freshBest, 7U);
}

TEST(StateDumpTest, Load_RefusesADumpNamingAnUnknownKind)
{
    const antwika::testing::ScratchFile file(
        "antwika_tower_defence_store_bad_kind.json");
    const auto path = file.path().string();

    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state["battle"]["mobs"][0]["kind"] = "dragon";

    const antwika::console::SnapshotFormat format(
        {.magic = antwika::tower_defence::kStateDumpMagic,
         .version = antwika::tower_defence::kStateDumpVersion},
        "antwika tower defence state dump document",
        antwika::tower_defence::standardStateDumpMigrations);
    format.write(
        antwika::console::Snapshot{.console = {}, .state = state},
        path);

    Campaign campaign(tinyCampaign());
    std::uint64_t best = 0;
    TowerDefenceSnapshotStore store(campaign, best);

    EXPECT_THROW(
        (void)store.load(path), antwika::console::SnapshotError);
}

TEST(StateDumpTest, Load_RefusesADumpThatDoesNotFitTheLevel)
{
    const antwika::testing::ScratchFile file(
        "antwika_tower_defence_store_bad_path_index.json");
    const auto path = file.path().string();

    Campaign campaign(tinyCampaign());
    std::uint64_t best = 0;
    TowerDefenceSnapshotStore store(campaign, best);

    CampaignMemory memory = campaign.remember();
    memory.battle.mobs.push_back(Mob{
        .id = 0,
        .kind = MobKind::Grunt,
        .pathIndex = 100000,
        .health = 1,
        .ticksUntilStep = 0});

    const antwika::console::SnapshotFormat format(
        {.magic = antwika::tower_defence::kStateDumpMagic,
         .version = antwika::tower_defence::kStateDumpVersion},
        "antwika tower defence state dump document",
        antwika::tower_defence::standardStateDumpMigrations);
    format.write(
        antwika::console::Snapshot{
            .console = {},
            .state = antwika::tower_defence::stateDumpToJson(
                StateDump{.campaign = memory, .bestScore = 0})},
        path);

    EXPECT_THROW(
        (void)store.load(path), antwika::console::SnapshotError);
}
