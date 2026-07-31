#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::GameState;
using antwika::game::GameSummary;
using antwika::game::GridExtent;
using antwika::game::kSaveFormatVersion;
using antwika::replay::kSchemaVersionKey;
using antwika::game::PathIndex;
using antwika::game::pathIndexOf;
using antwika::game::Point;
using antwika::game::SaveFormatError;
using antwika::game::SaveGame;
using antwika::game::saveGameFromJson;
using antwika::game::saveGameOf;
using antwika::game::saveGameToJson;
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

    EXPECT_EQ(saveGameFromJson(encoded), populated());
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

// The camera clamps a zoom level it cannot honour.
// A save asking for a level that does not exist loads at the closest.
// Which beats reading past the end of the table.
TEST(SaveGameTest, ClampsAnOutOfRangeZoomLevel)
{
    auto encoded = saveGameToJson(populated());
    encoded["camera"]["zoomLevel"] = 99;

    EXPECT_EQ(saveGameFromJson(encoded).camera.zoomLevel(),
              Camera({}, 99).zoomLevel());
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
            .kind = BuildingKind::FoodSource, .risk = 3});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 1, .y = 1});
    world.add<Walker>(
        walker,
        Walker{
            .kind = WalkerKind::Food, .carried = 12});
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
        save.buildings[0].kind, BuildingKind::FoodSource);
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

    world.add<Building>(
        source, Building{.walker = walker});
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{},
        GridExtent{.width = 8, .height = 8});

    ASSERT_EQ(save.walkers.size(), 1U);
    ASSERT_EQ(save.buildings.size(), 1U);
    EXPECT_EQ(save.walkers[0].home, 0U);
    EXPECT_EQ(save.buildings[0].walker, 0U);
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
        .kind = WalkerKind::Architect,
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
        .at = {.x = 3, .y = 4}, .walker = 0U}};

    const auto back = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(back.walkers[0].home, 0U);
    EXPECT_EQ(back.buildings[0].walker, 0U);
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
        .at = {.x = 3, .y = 4}, .walker = 7U}};

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
        antwika::game::SavedBuilding{.at = {.x = 3, .y = 3}, .walker = 1U}};

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
        .kind = WalkerKind::Water,
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
        .kind = BuildingKind::FoodSource,
        .stock = {3, 4},
        .risk = 5,
        .ticksUntilSpawn = 6,
        .ticksUntilDrain = 7,
        .ticksUntilRisk = 8,
        .walker = 9U};

    EXPECT_EQ(base, base);

    auto moved = base;
    moved.at = Cell{.x = 9, .y = 9};
    EXPECT_NE(base, moved);

    auto other = base;
    other.kind = BuildingKind::House;
    EXPECT_NE(base, other);

    auto stocked = base;
    stocked.stock = {0, 0};
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
    alone.walker.reset();
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
