#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

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
    // Small enough that the solver is not what these cases cost.
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

    // A dump exercising every member, one of them per mob kind.
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

    // Steps a campaign until at least one mob is walking.
    void stepUntilAMobWalks(Campaign &campaign)
    {
        while (campaign.battle().mobs().empty())
        {
            campaign.step();
        }
    }
} // namespace

TEST(StateDumpTest, TheDumpRoundTripsThroughTheCodec)
{
    const StateDump dump = fullDump();

    const auto decoded =
        antwika::tower_defence::stateDumpFromJson(
            antwika::tower_defence::stateDumpToJson(dump));

    EXPECT_EQ(decoded, dump);
}

TEST(StateDumpTest, EveryPhaseRoundTripsByItsPersistedName)
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

TEST(StateDumpTest, AnUnknownMobKindIsRefused)
{
    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state["battle"]["mobs"][0]["kind"] = "dragon";

    EXPECT_THROW(
        (void)antwika::tower_defence::stateDumpFromJson(state),
        StateDumpError);
}

TEST(StateDumpTest, AnUnknownPhaseIsRefused)
{
    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state["phase"] = "resting";

    EXPECT_THROW(
        (void)antwika::tower_defence::stateDumpFromJson(state),
        StateDumpError);
}

TEST(StateDumpTest, AMissingMemberFailsTheSchema)
{
    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state.erase("lives");

    EXPECT_THROW(
        (void)antwika::tower_defence::stateDumpFromJson(state),
        StateDumpError);
}

TEST(StateDumpTest, ADumpCarryingALevelIsNoDumpOfOurs)
{
    // The level is regenerated from the seed, never written down.
    // A document carrying one was written by no build of this app.
    auto state =
        antwika::tower_defence::stateDumpToJson(fullDump());
    state["levelData"] = nlohmann::json::object();

    EXPECT_THROW(
        (void)antwika::tower_defence::stateDumpFromJson(state),
        StateDumpError);
}

TEST(StateDumpTest, TheMigrationChainHasNoStepToTakeYet)
{
    const auto chain =
        antwika::tower_defence::standardStateDumpMigrations();

    nlohmann::json document;
    document["version"] = antwika::tower_defence::kStateDumpVersion;
    const auto before = document;
    chain.migrate(document);

    EXPECT_EQ(document, before);
}

TEST(StateDumpTest, ARememberedCampaignRestoresIntoAFreshOne)
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

TEST(StateDumpTest, RestoreRegeneratesTheLevelFromTheSeed)
{
    // Fought to its second level live, with no tower in the way.
    Campaign played(tinyCampaign());
    while (played.levelIndex() == 0)
    {
        played.step();
        ASSERT_LT(played.ticks(), 500U);
    }

    const CampaignMemory memory = played.remember();
    ASSERT_EQ(memory.level, 1U);

    // The dump carries no level, so the restore has to rebuild it.
    // The same (seed, index) must land on the same layout.
    Campaign fresh(tinyCampaign());
    ASSERT_TRUE(fresh.restore(memory));

    EXPECT_EQ(
        fresh.battle().level().path, played.battle().level().path);
    EXPECT_EQ(
        fresh.battle().level().tiles, played.battle().level().tiles);
}

TEST(StateDumpTest, AMobPastTheRegeneratedPathIsRefused)
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

    // Refused rather than repaired, and the campaign is untouched.
    EXPECT_FALSE(campaign.restore(memory));
    EXPECT_EQ(campaign.ticks(), before);
}

TEST(StateDumpTest, ALevelIndexNoCampaignCanReachIsRefused)
{
    Campaign campaign(tinyCampaign());

    CampaignMemory memory = campaign.remember();
    memory.level = tinyCampaign().levels.size() + 1;

    EXPECT_FALSE(campaign.restore(memory));
}

TEST(StateDumpTest, AWonCampaignRestoresPastItsLastLevel)
{
    Campaign campaign(tinyCampaign());

    // One past the end is exactly where a won campaign stands.
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

TEST(StateDumpTest, TheStoreDumpsAndLoadsTheWholeState)
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

TEST(StateDumpTest, TheStoreRefusesADumpNamingAnUnknownKind)
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

TEST(StateDumpTest, TheStoreRefusesADumpThatDoesNotFitTheLevel)
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
