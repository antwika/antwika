#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::Camera;
using antwika::game::kCoverageFull;
using antwika::game::kServiceCount;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::Errand;
using antwika::game::ErrandLeg;
using antwika::game::GameState;
using antwika::game::GameSummary;
using antwika::game::GridExtent;
using antwika::game::kSaveFormatVersion;
using antwika::game::kTicksPerStep;
using antwika::game::kZoomHalfWidths;
using antwika::replay::kSchemaVersionKey;
using antwika::game::PathIndex;
using antwika::game::pathIndexOf;
using antwika::game::Point;
using antwika::game::SaveFormatError;
using antwika::game::SaveGame;
using antwika::game::saveGameFromJson;
using antwika::game::saveGameOf;
using antwika::game::saveGameToJson;
using antwika::game::Production;
using antwika::game::Resource;
using antwika::game::SavedErrand;
using antwika::game::SavedWalker;
using antwika::game::Walker;
using antwika::game::WalkerKind;
using antwika::log::mocks::MockLogger;

namespace
{
    // A session with something of everything in it.
    // So a round trip that drops a field cannot match a default.
    SaveGame populated()
    {
        SaveGame save;
        save.state = GameState{.ticksProcessed = 42, .score = 7};
        save.extent = GridExtent{.width = 32, .height = 24};
        save.camera = Camera(Point{.x = -13, .y = 96}, 1);
        save.paths = {
            Cell{.x = 0, .y = 0},
            Cell{.x = 1, .y = 0},
            Cell{.x = 1, .y = 1},
        };
        save.walkers = {
            SavedWalker{.at = {.x = 1, .y = 0}, .facing = Direction::South},
            SavedWalker{.at = {.x = 0, .y = 0}, .facing = Direction::West},
        };
        save.seed = 0xF00DFACEULL;
        return save;
    }
} // namespace

TEST(SaveGameTest, RoundTripsEveryField)
{
    const auto original = populated();

    const auto loaded = saveGameFromJson(saveGameToJson(original));

    EXPECT_EQ(loaded.state, original.state);
    EXPECT_EQ(loaded.extent, original.extent);
    EXPECT_EQ(loaded.camera, original.camera);
    EXPECT_EQ(loaded.paths, original.paths);
    EXPECT_EQ(loaded.walkers, original.walkers);
    EXPECT_EQ(loaded.seed, original.seed);
    EXPECT_EQ(loaded, original);
}

TEST(SaveGameTest, RoundTripsAnEmptySession)
{
    const SaveGame original;

    EXPECT_EQ(saveGameFromJson(saveGameToJson(original)), original);
}

TEST(SaveGameTest, RoundTripsEveryDirection)
{
    SaveGame original;
    original.walkers = {
        SavedWalker{.at = {.x = 0, .y = 0}, .facing = Direction::North},
        SavedWalker{.at = {.x = 1, .y = 0}, .facing = Direction::East},
        SavedWalker{.at = {.x = 2, .y = 0}, .facing = Direction::South},
        SavedWalker{.at = {.x = 3, .y = 0}, .facing = Direction::West},
    };

    EXPECT_EQ(saveGameFromJson(saveGameToJson(original)), original);
}

// The key is antwika::replay's, not one of this format's own.
// Every persisted document in the code base states its version there.
TEST(SaveGameTest, WritesTheCurrentSchemaVersion)
{
    const auto encoded = saveGameToJson(populated());

    EXPECT_EQ(encoded.at(std::string(kSchemaVersionKey))
                  .get<std::uint32_t>(),
              kSaveFormatVersion);
    EXPECT_EQ(encoded.at("magic").get<std::string>(),
              "antwika-game-save");
}

TEST(SaveGameTest, WritesDirectionsByName)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 2, .y = 3}, .facing = Direction::North}};

    const auto encoded = saveGameToJson(save);

    EXPECT_EQ(encoded.at("walkers").at(0).at("facing").get<std::string>(),
              "north");
}

TEST(SaveGameTest, TreatsAnAbsentVersionAsVersionOne)
{
    auto encoded = saveGameToJson(populated());
    encoded.erase(std::string(kSchemaVersionKey));

    // Read as version 1, so it comes up through both migrations.
    // A version 1 walker carried none of the economy.
    // So each is filled in as one that has just set out.
    // And what version 2 called food a version 3 file sells.
    auto expected = populated();
    for (auto &walker : expected.walkers)
    {
        walker.kind = antwika::game::WalkerKind::MarketSeller;
    }

    EXPECT_EQ(saveGameFromJson(encoded), expected);
}

TEST(SaveGameTest, RejectsANewerVersion)
{
    auto encoded = saveGameToJson(populated());
    encoded[std::string(kSchemaVersionKey)] =
        kSaveFormatVersion + 1;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAnOlderVersionWithNoMigration)
{
    auto encoded = saveGameToJson(populated());
    encoded[std::string(kSchemaVersionKey)] = 0;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAVersionThatIsNotAnInteger)
{
    auto encoded = saveGameToJson(populated());
    encoded[std::string(kSchemaVersionKey)] = "one";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsADocumentThatIsNotAnObject)
{
    const nlohmann::json encoded = nlohmann::json::array({1, 2, 3});

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAMissingRequiredField)
{
    auto encoded = saveGameToJson(populated());
    encoded.erase("paths");

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAMissingNestedField)
{
    auto encoded = saveGameToJson(populated());
    encoded["camera"].erase("zoomLevel");

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAnUnexpectedField)
{
    auto encoded = saveGameToJson(populated());
    encoded["weather"] = nlohmann::json::array();

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAFieldOfTheWrongType)
{
    auto encoded = saveGameToJson(populated());
    encoded["seed"] = "lots";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsTheWrongMagic)
{
    auto encoded = saveGameToJson(populated());
    encoded["magic"] = "antwika-replay";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsADirectionThatIsNotOneOfTheFour)
{
    auto encoded = saveGameToJson(populated());
    encoded["walkers"].at(0)["facing"] = "widdershins";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsACoordinateOutsideAnInt32)
{
    auto encoded = saveGameToJson(populated());
    encoded["paths"].at(0)["x"] = 5'000'000'000LL;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// The largest phase a walker is ever in is kTicksPerStep - 1.
// WalkerSystem writes that and counts it down to zero.
// So a file above it names a walker no run ever produced.
TEST(SaveGameTest, RejectsAStepPhaseNoWalkerCouldBeIn)
{
    auto encoded = saveGameToJson(populated());
    encoded["walkers"].at(0)["ticksUntilStep"] = kTicksPerStep;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// The decode is get<std::uint8_t>(), and nlohmann narrows in silence.
// So a schema capped at what an int32 holds let 256 through as 0.
TEST(SaveGameTest, RejectsAStepPhaseThatWouldNarrowToAnotherNumber)
{
    auto encoded = saveGameToJson(populated());
    encoded["walkers"].at(0)["ticksUntilStep"] = 256;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// Camera clamps a level it cannot honour, and still does.
// A file is the one caller that is not owed that courtesy.
// A level past the end of the table is a camera nobody ever had.
// Loading it at the closest would be a session somebody never played.
TEST(SaveGameTest, RejectsAZoomLevelPastTheEndOfTheTable)
{
    auto encoded = saveGameToJson(populated());
    encoded["camera"]["zoomLevel"] = kZoomHalfWidths.size();

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// The boundary the refusal is drawn at, from the legal side.
TEST(SaveGameTest, ReadsTheClosestZoomLevelTheTableHolds)
{
    auto encoded = saveGameToJson(populated());
    encoded["camera"]["zoomLevel"] = kZoomHalfWidths.size() - 1;

    EXPECT_EQ(saveGameFromJson(encoded).camera.zoomLevel(),
              kZoomHalfWidths.size() - 1);
}

TEST(SaveGameTest, TakesASaveFromARunningSession)
{
    ::testing::NiceMock<MockLogger> logger;
    World world(logger);
    PathIndex paths;
    paths.insert(Cell{.x = 1, .y = 1});

    const auto source = world.create();
    world.add<Cell>(source, Cell{.x = 4, .y = 4});
    world.add<Building>(
        source,
        Building{
            .kind = BuildingKind::Farm, .risk = 3});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 1, .y = 1});
    world.add<Walker>(
        walker,
        Walker{
            .kind = WalkerKind::MarketSeller, .carried = 12});
    world.commit();

    const Camera camera(antwika::gfx::Point{.x = 9, .y = 8}, 2);
    const GameState state{.ticksProcessed = 5, .score = 6};

    const auto save = saveGameOf(
        world, paths, camera, state, GridExtent{.width = 8, .height = 8}, 7);

    EXPECT_EQ(save.state, state);
    EXPECT_EQ(save.camera, camera);
    EXPECT_EQ(save.seed, 7U);
    EXPECT_EQ(save.paths, (std::vector<Cell>{{.x = 1, .y = 1}}));

    ASSERT_EQ(save.walkers.size(), 1U);
    EXPECT_EQ(save.walkers[0].carried, 12);

    ASSERT_EQ(save.buildings.size(), 1U);
    EXPECT_EQ(save.buildings[0].risk, 3);
    EXPECT_EQ(
        save.buildings[0].kind, BuildingKind::Farm);
}

// The pair is what the reader checks a file for.
// So writing it has to name both ends.
// Otherwise a round trip would refuse its own output.
TEST(SaveGameTest, WritesTheBuildingAndWalkerLinkFromBothEnds)
{
    ::testing::NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto source = world.create();
    world.add<Cell>(source, Cell{.x = 4, .y = 4});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 1, .y = 1});
    world.add<Walker>(walker, Walker{});

    world.add<Building>(source, Building{.walkers = {walker}});
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{},
        GridExtent{.width = 8, .height = 8});

    ASSERT_EQ(save.walkers.size(), 1U);
    ASSERT_EQ(save.buildings.size(), 1U);
    EXPECT_EQ(save.walkers[0].home, 0U);
    EXPECT_EQ(
        save.buildings[0].walkers, (std::vector<std::size_t>{0U}));
}

TEST(SaveGameTest, RebuildsThePathIndex)
{
    const auto index = pathIndexOf(populated());

    EXPECT_EQ(index.size(), 3U);
    EXPECT_TRUE(index.has(Cell{.x = 1, .y = 1}));
    EXPECT_FALSE(index.has(Cell{.x = 9, .y = 9}));
}

TEST(SaveGameTest, RoundTripsABuildingsWholeState)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4},
        .kind = BuildingKind::FireStation,
        .stock = {11, 22},
        .risk = 33,
        .ticksUntilSpawn = 44,
        .ticksUntilDrain = 55,
        .ticksUntilRisk = 66}};

    EXPECT_EQ(saveGameFromJson(saveGameToJson(save)), save);
}

TEST(SaveGameTest, RoundTripsAWalkersWholeState)
{
    SaveGame save;
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .facing = Direction::South,
        .kind = WalkerKind::Engineer,
        .carried = 17,
        .stepsUntilHome = 5,
        .ticksUntilStep = 1}};

    EXPECT_EQ(saveGameFromJson(saveGameToJson(save)), save);
}

TEST(SaveGameTest, RoundTripsTheLinkBetweenABuildingAndItsWalker)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}, .home = 0U}};
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4}, .walkers = {0U}}};

    const auto back = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(back.walkers[0].home, 0U);
    EXPECT_EQ(back.buildings[0].walkers, (std::vector<std::size_t>{0U}));
}

// A market has a buyer and a seller out at once.
// Which is the whole reason the link became a list.
TEST(SaveGameTest, RoundTripsABuildingWithTwoWalkersOut)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 1, .y = 2}, .home = 0U},
        SavedWalker{.at = {.x = 2, .y = 2}, .home = 0U}};
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4}, .walkers = {0U, 1U}}};

    const auto back = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(back, save);
    EXPECT_EQ(
        back.buildings[0].walkers, (std::vector<std::size_t>{0U, 1U}));
}

// A building may hold kMaxWalkersOut, and no more.
// A file naming a third names a slot this build has not got.
// Which is a file to refuse rather than one to read two of.
TEST(SaveGameTest, RejectsABuildingWithMoreWalkersThanItHasSlots)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 1, .y = 1}, .home = 0U},
        SavedWalker{.at = {.x = 2, .y = 2}, .home = 0U},
        SavedWalker{.at = {.x = 3, .y = 3}, .home = 0U}};
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4}, .walkers = {0U, 1U, 2U}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAWalkerWhoseHomeIsNotABuildingInTheFile)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}, .home = 7U}};

    auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsABuildingWhoseWalkerIsNotAWalkerInTheFile)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4}, .walkers = {7U}}};

    auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// A pair that names each other is what the writer produces.
// So a file where only one end points is one somebody edited.
TEST(SaveGameTest, RejectsAPairThatDisagreesAboutEachOther)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}, .home = 0U}};
    save.buildings = {antwika::game::SavedBuilding{.at = {.x = 3, .y = 4}}};

    auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAWalkerKindThisBuildDoesNotHave)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}}};

    auto encoded = saveGameToJson(save);
    encoded["walkers"][0]["kind"] = "juggler";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsABuildingKindThisBuildDoesNotHave)
{
    SaveGame save;
    save.buildings = {
        antwika::game::SavedBuilding{.at = {.x = 3, .y = 4}}};

    auto encoded = saveGameToJson(save);
    encoded["buildings"][0]["kind"] = "tower";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// The building names a walker other than the one naming it.
// Which is the second way a pair can disagree.
TEST(SaveGameTest, RejectsABuildingPointingAtSomebodyElsesWalker)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 1, .y = 1}, .home = 0U},
        SavedWalker{.at = {.x = 2, .y = 2}}};
    save.buildings = {
        antwika::game::SavedBuilding{
            .at = {.x = 3, .y = 3}, .walkers = {1U}}};

    auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// The defaulted comparisons short-circuit.
// So every field needs a pair that differs in it alone.
TEST(SaveGameTest, SavedWalkerEqualityComparesEveryField)
{
    const SavedWalker base{
        .at = {.x = 1, .y = 2},
        .facing = Direction::North,
        .kind = WalkerKind::WaterCarrier,
        .carried = 3,
        .stepsUntilHome = 4,
        .ticksUntilStep = 1,
        .home = 5U};

    EXPECT_EQ(base, base);

    auto moved = base;
    moved.at = Cell{.x = 9, .y = 9};
    EXPECT_NE(base, moved);

    auto turned = base;
    turned.facing = Direction::South;
    EXPECT_NE(base, turned);

    auto other = base;
    other.kind = WalkerKind::Fireman;
    EXPECT_NE(base, other);

    auto emptied = base;
    emptied.carried = 0;
    EXPECT_NE(base, emptied);

    auto tired = base;
    tired.stepsUntilHome = 0;
    EXPECT_NE(base, tired);

    auto later = base;
    later.ticksUntilStep = 0;
    EXPECT_NE(base, later);

    auto homeless = base;
    homeless.home.reset();
    EXPECT_NE(base, homeless);
}

TEST(SaveGameTest, SavedBuildingEqualityComparesEveryField)
{
    const antwika::game::SavedBuilding base{
        .at = {.x = 1, .y = 2},
        .kind = BuildingKind::Farm,
        .stock = {3, 4, 5},
        .risk = 5,
        .ticksUntilSpawn = 6,
        .ticksUntilDrain = 7,
        .ticksUntilRisk = 8,
        .walkers = {9U}};

    EXPECT_EQ(base, base);

    auto moved = base;
    moved.at = Cell{.x = 9, .y = 9};
    EXPECT_NE(base, moved);

    auto other = base;
    other.kind = BuildingKind::House;
    EXPECT_NE(base, other);

    auto stocked = base;
    stocked.stock = {0, 0, 0};
    EXPECT_NE(base, stocked);

    auto risky = base;
    risky.risk = 0;
    EXPECT_NE(base, risky);

    auto spawning = base;
    spawning.ticksUntilSpawn = 0;
    EXPECT_NE(base, spawning);

    auto draining = base;
    draining.ticksUntilDrain = 0;
    EXPECT_NE(base, draining);

    auto rising = base;
    rising.ticksUntilRisk = 0;
    EXPECT_NE(base, rising);

    auto alone = base;
    alone.walkers.clear();
    EXPECT_NE(base, alone);
}

TEST(SaveGameTest, SaveEqualityComparesTheBuildings)
{
    SaveGame base;
    base.buildings = {
        antwika::game::SavedBuilding{.at = {.x = 1, .y = 1}}};

    auto other = base;
    other.buildings.clear();

    EXPECT_EQ(base, base);
    EXPECT_NE(base, other);
}

// The additive member, and what its absence means.
// A version-3 file written before coverage existed names none.
// Which is the very thing a building nothing has reached holds.
TEST(SaveGameTest, RoundTripsABuildingsCoverage)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4},
        .kind = BuildingKind::House,
        .coverage = {kCoverageFull, 0, 12, 3}}};

    const auto encoded = saveGameToJson(save);

    ASSERT_TRUE(encoded.at("buildings").at(0).contains("coverage"));
    EXPECT_EQ(saveGameFromJson(encoded), save);
}

TEST(SaveGameTest, WritesNoCoverageForABuildingNothingHasReached)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{.at = {.x = 1, .y = 1}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("coverage"));
    EXPECT_EQ(saveGameFromJson(encoded).buildings[0].coverage,
              (std::array<std::int32_t, kServiceCount>{}));
}

// A countdown above the full one is longer than any walker leaves.
// So a file naming one is a session nobody ever played.
TEST(SaveGameTest, RejectsCoverageAboveWhatAWalkerCouldEverLeave)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 1, .y = 1}, .coverage = {kCoverageFull + 1, 0, 0, 0}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RoundTripsAWalkerMidErrand)
{
    SaveGame save;
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .kind = WalkerKind::CartPusher,
        .carried = 30,
        .home = 0U,
        .errand =
            SavedErrand{
                .destination = 1U,
                .carrying = Resource::Pottery,
                .leg = ErrandLeg::Returning}}};
    save.buildings = {
        antwika::game::SavedBuilding{
            .at = {.x = 4, .y = 4},
            .kind = BuildingKind::Workshop,
            .walkers = {0U}},
        antwika::game::SavedBuilding{
            .at = {.x = 8, .y = 8}, .kind = BuildingKind::Storage}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.walkers, save.walkers);
}

// An errand naming nowhere is an ordinary state, not a missing field.
TEST(SaveGameTest, RoundTripsAnErrandBoundNowhere)
{
    SaveGame save;
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .kind = WalkerKind::MarketSeller,
        .errand = SavedErrand{.carrying = Resource::Food}}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    ASSERT_EQ(loaded.walkers.size(), 1U);
    ASSERT_TRUE(loaded.walkers[0].errand.has_value());
    EXPECT_FALSE(loaded.walkers[0].errand->destination.has_value());
}

TEST(SaveGameTest, RoundTripsAProducersCountdown)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4},
        .kind = BuildingKind::Farm,
        .ticksUntilOutput = 11}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.buildings, save.buildings);
}

TEST(SaveGameTest, LeavesABuildingThatNeverProducedWithoutACountdown)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("ticksUntilOutput"));
    EXPECT_FALSE(
        saveGameFromJson(encoded).buildings[0].ticksUntilOutput.has_value());
}

// Refused rather than repaired, exactly as a walker's home is.
// A repaired save is a session somebody never had.
TEST(SaveGameTest, RejectsAnErrandWhoseDestinationIsNotABuildingInIt)
{
    SaveGame save;
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .errand = SavedErrand{.destination = 7U}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsACoverageArrayThatIsNotOnePerService)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{.at = {.x = 1, .y = 1}}};

    auto encoded = saveGameToJson(save);
    encoded["buildings"][0]["coverage"] = {1, 2};

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, RejectsAnErrandNamingAResourceThisBuildDoesNotHave)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 1, .y = 2}, .errand = SavedErrand{}}};

    auto encoded = saveGameToJson(save);
    encoded["walkers"][0]["errand"]["carrying"] = "amphorae";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// A hand-written document of the version this build writes.
// From before coverage was a thing anybody stored.
// Step four of docs/schema-versioning.md, for an additive member.
TEST(SaveGameTest, ReadsAVersionThreeDocumentWrittenBeforeCoverage)
{
    const auto document = nlohmann::json::parse(R"({
        "magic": "antwika-game-save",
        "version": 3,
        "state": {"ticksProcessed": 9, "score": 2},
        "extent": {"width": 8, "height": 8},
        "camera": {"panX": 1, "panY": 2, "zoomLevel": 1},
        "paths": [{"x": 0, "y": 0}],
        "walkers": [],
        "buildings": [
            {"x": 3, "y": 4, "kind": "house", "stock": [1, 2, 3],
             "risk": 5, "ticksUntilSpawn": 6, "ticksUntilDrain": 7,
             "ticksUntilRisk": 8}
        ],
        "seed": 11
    })");

    const auto save = saveGameFromJson(document);

    ASSERT_EQ(save.buildings.size(), 1U);
    EXPECT_EQ(save.buildings[0].coverage,
              (std::array<std::int32_t, kServiceCount>{}));
    EXPECT_EQ(save.buildings[0].risk, 5);
}

// Coverage is read out of the World like every other piece of state.
TEST(SaveGameTest, TakesEachBuildingsCoverageFromTheWorld)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};
    const PathIndex paths;

    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 2, .y = 2});
    world.add<Building>(house, Building{.kind = BuildingKind::House});
    antwika::game::setCoverage(
        world, house, antwika::game::Coverage{.ticksLeft = {4, 5, 6, 7}});
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{},
        GridExtent{.width = 8, .height = 8});

    ASSERT_EQ(save.buildings.size(), 1U);
    EXPECT_EQ(save.buildings[0].coverage,
              (std::array<std::int32_t, kServiceCount>{4, 5, 6, 7}));
}

// The member has to be in the comparison.
// Or a round trip that dropped it would still match.
TEST(SaveGameTest, SavedBuildingEqualityComparesTheCoverage)
{
    const antwika::game::SavedBuilding base{
        .at = {.x = 1, .y = 1}, .coverage = {1, 2, 3, 4}};

    EXPECT_EQ(base, base);

    for (std::size_t slot = 0; slot < kServiceCount; ++slot)
    {
        auto changed = base;
        changed.coverage[slot] += 1;

        EXPECT_NE(base, changed);
    }
}

TEST(SaveGameTest, RejectsAnErrandNamingALegThisBuildDoesNotHave)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 1, .y = 2}, .errand = SavedErrand{}}};

    auto encoded = saveGameToJson(save);
    encoded["walkers"][0]["errand"]["leg"] = "sideways";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, TakesAnErrandAndACountdownFromARunningSession)
{
    ::testing::NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto farm = world.create();
    world.add<Cell>(farm, Cell{.x = 4, .y = 4});
    world.add<Building>(farm, Building{.kind = BuildingKind::Farm});
    world.add<Production>(farm, Production{.ticksUntilOutput = 13});

    const auto store = world.create();
    world.add<Cell>(store, Cell{.x = 8, .y = 8});
    world.add<Building>(store, Building{.kind = BuildingKind::Storage});

    const auto cart = world.create();
    world.add<Cell>(cart, Cell{.x = 1, .y = 1});
    world.add<Walker>(cart, Walker{.kind = WalkerKind::CartPusher});
    world.add<Errand>(
        cart,
        Errand{.destination = store, .carrying = Resource::Food});
    world.commit();

    const auto save = saveGameOf(
        world,
        paths,
        Camera{},
        GameState{},
        GridExtent{.width = 16, .height = 16});

    ASSERT_EQ(save.buildings.size(), 2U);
    EXPECT_EQ(save.buildings[0].ticksUntilOutput, 13);
    EXPECT_FALSE(save.buildings[1].ticksUntilOutput.has_value());

    ASSERT_EQ(save.walkers.size(), 1U);
    ASSERT_TRUE(save.walkers[0].errand.has_value());
    EXPECT_EQ(save.walkers[0].errand->destination, 1U);
}

// A destination whose building never reached the file is nowhere.
// Which is the same state a cart with no store to go to already has.
TEST(SaveGameTest, WritesAnErrandNamingNobodyWhenItsStoreIsGone)
{
    ::testing::NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto cart = world.create();
    world.add<Cell>(cart, Cell{.x = 1, .y = 1});
    world.add<Walker>(cart, Walker{.kind = WalkerKind::CartPusher});
    world.add<Errand>(
        cart, Errand{.destination = static_cast<antwika::ecs::Entity>(99)});
    world.commit();

    const auto save = saveGameOf(
        world,
        paths,
        Camera{},
        GameState{},
        GridExtent{.width = 16, .height = 16});

    ASSERT_EQ(save.walkers.size(), 1U);
    ASSERT_TRUE(save.walkers[0].errand.has_value());
    EXPECT_FALSE(save.walkers[0].errand->destination.has_value());
}

TEST(SaveGameTest, SavedErrandEqualityComparesEveryField)
{
    const SavedErrand base{
        .destination = 1U,
        .carrying = Resource::Clay,
        .leg = ErrandLeg::Returning};

    EXPECT_EQ(base, base);

    auto lost = base;
    lost.destination.reset();
    EXPECT_NE(base, lost);

    auto other = base;
    other.carrying = Resource::Food;
    EXPECT_NE(base, other);

    auto turned = base;
    turned.leg = ErrandLeg::Outbound;
    EXPECT_NE(base, turned);
}

TEST(SaveGameTest, SavedWalkerEqualityComparesItsErrand)
{
    SavedWalker base{.at = {.x = 1, .y = 2}};
    base.errand = SavedErrand{};

    auto roaming = base;
    roaming.errand.reset();

    EXPECT_EQ(base, base);
    EXPECT_NE(base, roaming);
}

TEST(SaveGameTest, SavedBuildingEqualityComparesItsCountdown)
{
    antwika::game::SavedBuilding base{.at = {.x = 1, .y = 2}};
    base.ticksUntilOutput = 4;

    auto idle = base;
    idle.ticksUntilOutput.reset();

    EXPECT_EQ(base, base);
    EXPECT_NE(base, idle);
}

// A city holds more buildings than the smallest case a decoder sees.
// And the arrays that hold them grow as one is read.
TEST(SaveGameTest, RoundTripsACityOfSeveralBuildings)
{
    SaveGame save;

    for (std::int32_t index = 0; index < 5; ++index)
    {
        save.buildings.push_back(
            antwika::game::SavedBuilding{
                .at = {.x = index, .y = index},
                .kind = BuildingKind::Farm,
                .ticksUntilOutput = index});
    }

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.buildings, save.buildings);
}

TEST(SaveGameTest, TakesACityOfSeveralBuildingsFromARunningSession)
{
    ::testing::NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    for (std::int32_t index = 0; index < 5; ++index)
    {
        const auto entity = world.create();
        world.add<Cell>(entity, Cell{.x = index, .y = index});
        world.add<Building>(entity, Building{.kind = BuildingKind::Farm});
    }

    world.commit();

    const auto save = saveGameOf(
        world,
        paths,
        Camera{},
        GameState{},
        GridExtent{.width = 16, .height = 16});

    EXPECT_EQ(save.buildings.size(), 5U);
}

// A house part-way to its next level, countdowns and all.
// They are persisted rather than reset for Building's three's reason.
// Two houses reopened with one number grow in lockstep from then on.
TEST(SaveGameTest, RoundTripsAHouseMidEvolution)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4},
        .kind = BuildingKind::House,
        .household =
            antwika::game::Household{
                .level = antwika::game::HousingLevel::Hovel,
                .ticksUntilEvolve = 17,
                .ticksUntilDevolve = 23,
                .population = 4}}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.buildings, save.buildings);
}

TEST(SaveGameTest, WritesTheHousingLevelByName)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4},
        .kind = BuildingKind::House,
        .household =
            antwika::game::Household{
                .level = antwika::game::HousingLevel::Cottage}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_EQ(
        encoded.at("buildings").at(0).at("household").at("level"),
        "cottage");
}

TEST(SaveGameTest, LeavesAHouseThatNeverGrewWithoutAHousehold)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("household"));
    EXPECT_FALSE(
        saveGameFromJson(encoded).buildings[0].household.has_value());
}

TEST(SaveGameTest, RejectsAHousingLevelThisBuildDoesNotHave)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 1, .y = 1},
        .kind = BuildingKind::House,
        .household = antwika::game::Household{}}};

    auto encoded = saveGameToJson(save);
    encoded["buildings"][0]["household"]["level"] = "palace";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// All four members or none, so a half-written household is refused.
// They only ever mean anything together -- see SaveHousing.cpp.
TEST(SaveGameTest, RejectsAHouseholdMissingOneOfItsMembers)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 1, .y = 1},
        .kind = BuildingKind::House,
        .household = antwika::game::Household{}}};

    auto encoded = saveGameToJson(save);
    encoded["buildings"][0]["household"].erase("population");

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

// A hand-written document of the version this build writes.
// From before housing was a thing anybody stored.
// Step four of docs/schema-versioning.md, for an additive member.
TEST(SaveGameTest, ReadsAVersionThreeDocumentWrittenBeforeHousing)
{
    const auto document = nlohmann::json::parse(R"({
        "magic": "antwika-game-save",
        "version": 3,
        "state": {"ticksProcessed": 4, "score": 1},
        "extent": {"width": 8, "height": 8},
        "camera": {"panX": 0, "panY": 0, "zoomLevel": 0},
        "paths": [],
        "walkers": [],
        "buildings": [
            {"x": 2, "y": 2, "kind": "house", "stock": [9, 0, 0],
             "risk": 0, "ticksUntilSpawn": 1, "ticksUntilDrain": 2,
             "ticksUntilRisk": 3}
        ],
        "seed": 5
    })");

    const auto save = saveGameFromJson(document);

    ASSERT_EQ(save.buildings.size(), 1U);
    EXPECT_FALSE(save.buildings[0].household.has_value());
}

// Read out of the World like every other piece of state.
TEST(SaveGameTest, TakesEachHousesHouseholdFromTheWorld)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};
    const PathIndex paths;

    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 2, .y = 2});
    world.add<Building>(house, Building{.kind = BuildingKind::House});
    antwika::game::setHousehold(
        world,
        house,
        antwika::game::Household{
            .level = antwika::game::HousingLevel::Shack,
            .ticksUntilEvolve = 3,
            .ticksUntilDevolve = 4,
            .population = 5});

    const auto well = world.create();
    world.add<Cell>(well, Cell{.x = 6, .y = 6});
    world.add<Building>(well, Building{.kind = BuildingKind::Well});
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{},
        GridExtent{.width = 8, .height = 8});

    ASSERT_EQ(save.buildings.size(), 2U);
    ASSERT_TRUE(save.buildings[0].household.has_value());
    EXPECT_EQ(
        save.buildings[0].household->level,
        antwika::game::HousingLevel::Shack);
    EXPECT_EQ(save.buildings[0].household->population, 5);
    EXPECT_FALSE(save.buildings[1].household.has_value());
}

// The member has to be in the comparison.
// Or a round trip that dropped it would still match.
TEST(SaveGameTest, SavedBuildingEqualityComparesTheHousehold)
{
    const antwika::game::SavedBuilding base{
        .at = {.x = 1, .y = 1},
        .household = antwika::game::Household{
            .level = antwika::game::HousingLevel::Hovel}};

    EXPECT_EQ(base, base);

    auto homeless = base;
    homeless.household.reset();
    EXPECT_NE(base, homeless);

    auto grown = base;
    grown.household->level = antwika::game::HousingLevel::Cottage;
    EXPECT_NE(base, grown);
}
