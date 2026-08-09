#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/FireCall.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Workforce.hpp"

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
using antwika::game::SavedJourney;
using antwika::game::SavedWalker;
using antwika::game::Walker;
using antwika::game::WalkerKind;
using antwika::log::mocks::MockLogger;

namespace
{
    SaveGame populated()
    {
        SaveGame save;
        save.state =
            GameState{.ticksProcessed = 42, .score = 7, .money = -125};
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
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsEveryField)
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

TEST(SaveGameTest, SaveGameFromJson_RoundTripsAnEmptySession)
{
    const SaveGame original;

    EXPECT_EQ(saveGameFromJson(saveGameToJson(original)), original);
}

TEST(SaveGameTest, SaveGameFromJson_ReadsAbsentMoneyAsTheBank)
{
    auto encoded = saveGameToJson(populated());
    encoded.at("state").erase("money");

    const auto loaded = saveGameFromJson(encoded);

    EXPECT_EQ(loaded.state.money, antwika::game::kStartingMoney);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsEveryDirection)
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

TEST(SaveGameTest, SaveGameToJson_WritesTheCurrentSchemaVersion)
{
    const auto encoded = saveGameToJson(populated());

    EXPECT_EQ(encoded.at(std::string(kSchemaVersionKey))
                  .get<std::uint32_t>(),
              kSaveFormatVersion);
    EXPECT_EQ(encoded.at("magic").get<std::string>(),
              "antwika-game-save");
}

TEST(SaveGameTest, SaveGameToJson_WritesDirectionsByName)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 2, .y = 3}, .facing = Direction::North}};

    const auto encoded = saveGameToJson(save);

    EXPECT_EQ(encoded.at("walkers").at(0).at("facing").get<std::string>(),
              "north");
}

TEST(SaveGameTest, SaveGameFromJson_ReadsNoVersionAsOne)
{
    auto encoded = saveGameToJson(populated());
    encoded.erase(std::string(kSchemaVersionKey));

    auto expected = populated();
    for (auto &walker : expected.walkers)
    {
        walker.kind = antwika::game::WalkerKind::MarketSeller;
    }

    EXPECT_EQ(saveGameFromJson(encoded), expected);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsANewerVersion)
{
    auto encoded = saveGameToJson(populated());
    encoded[std::string(kSchemaVersionKey)] =
        kSaveFormatVersion + 1;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAnUnmigratedVersion)
{
    auto encoded = saveGameToJson(populated());
    encoded[std::string(kSchemaVersionKey)] = 0;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsANonIntegerVersion)
{
    auto encoded = saveGameToJson(populated());
    encoded[std::string(kSchemaVersionKey)] = "one";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsADocumentThatIsNotAnObject)
{
    const nlohmann::json encoded = nlohmann::json::array({1, 2, 3});

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAMissingRequiredField)
{
    auto encoded = saveGameToJson(populated());
    encoded.erase("paths");

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAMissingNestedField)
{
    auto encoded = saveGameToJson(populated());
    encoded["camera"].erase("zoomLevel");

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAnUnexpectedField)
{
    auto encoded = saveGameToJson(populated());
    encoded["weather"] = nlohmann::json::array();

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAFieldOfTheWrongType)
{
    auto encoded = saveGameToJson(populated());
    encoded["seed"] = "lots";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsTheWrongMagic)
{
    auto encoded = saveGameToJson(populated());
    encoded["magic"] = "antwika-replay";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAnUnknownDirection)
{
    auto encoded = saveGameToJson(populated());
    encoded["walkers"].at(0)["facing"] = "widdershins";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsACoordinateOutsideAnInt32)
{
    auto encoded = saveGameToJson(populated());
    encoded["paths"].at(0)["x"] = 5'000'000'000LL;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAnImpossiblePhase)
{
    auto encoded = saveGameToJson(populated());
    encoded["walkers"].at(0)["ticksUntilStep"] = kTicksPerStep;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsANarrowingStepPhase)
{
    auto encoded = saveGameToJson(populated());
    encoded["walkers"].at(0)["ticksUntilStep"] = 256;

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAZoomPastTheTable)
{
    auto encoded = saveGameToJson(populated());
    encoded["camera"]["zoomLevel"] = kZoomHalfWidths.size();

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_ReadsTheClosestZoomLevel)
{
    auto encoded = saveGameToJson(populated());
    encoded["camera"]["zoomLevel"] = kZoomHalfWidths.size() - 1;

    EXPECT_EQ(saveGameFromJson(encoded).camera.zoomLevel(),
              kZoomHalfWidths.size() - 1);
}

TEST(SaveGameTest, SaveGameOf_TakesASaveFromARunningSession)
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
            .kind = BuildingKind::Farm,
            .fireRisk = 3,
            .collapseRisk = 4});

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
    EXPECT_EQ(save.buildings[0].collapseRisk, 4);
    EXPECT_EQ(
        save.buildings[0].kind, BuildingKind::Farm);
}

TEST(SaveGameTest, SaveGameOf_WritesTheLinkFromBothEnds)
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

TEST(SaveGameTest, PathIndexOf_RebuildsThePathIndex)
{
    const auto index = pathIndexOf(populated());

    EXPECT_EQ(index.size(), 3U);
    EXPECT_TRUE(index.has(Cell{.x = 1, .y = 1}));
    EXPECT_FALSE(index.has(Cell{.x = 9, .y = 9}));
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsABuildingsWholeState)
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

TEST(SaveGameTest, SaveGameFromJson_RoundTripsAWalkersWholeState)
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

TEST(SaveGameTest, SaveGameFromJson_KeepsABuildingWalkerLink)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}, .home = 0U}};
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4}, .walkers = {0U}}};

    const auto back = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(back.walkers[0].home, 0U);
    EXPECT_EQ(back.buildings[0].walkers, (std::vector<std::size_t>{0U}));
}

TEST(SaveGameTest, SaveGameFromJson_KeepsTwoWalkersOut)
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

TEST(SaveGameTest, SaveGameFromJson_RejectsTooManyWalkers)
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

TEST(SaveGameTest, SaveGameFromJson_RejectsAHomelessWalker)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}, .home = 7U}};

    auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAnAbsentWalker)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4}, .walkers = {7U}}};

    auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsADisagreeingPair)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}, .home = 0U}};
    save.buildings = {antwika::game::SavedBuilding{.at = {.x = 3, .y = 4}}};

    auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAnUnknownWalkerKind)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}}};

    auto encoded = saveGameToJson(save);
    encoded["walkers"][0]["kind"] = "juggler";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAnUnknownBuildingKind)
{
    SaveGame save;
    save.buildings = {
        antwika::game::SavedBuilding{.at = {.x = 3, .y = 4}}};

    auto encoded = saveGameToJson(save);
    encoded["buildings"][0]["kind"] = "tower";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsABorrowedWalker)
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

TEST(SaveGameTest, OperatorEquals_ComparesEverySavedWalkerField)
{
    const SavedWalker base{
        .at = {.x = 1, .y = 2},
        .facing = Direction::North,
        .kind = WalkerKind::WaterCarrier,
        .carried = 3,
        .stepsUntilHome = 4,
        .ticksUntilStep = 1,
        .home = 5U};

    const auto twin = base;
    EXPECT_EQ(base, twin);

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

TEST(SaveGameTest, OperatorEquals_ComparesEverySavedBuildingField)
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

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto moved = base;
    moved.at = Cell{.x = 9, .y = 9};
    EXPECT_NE(base, moved);

    auto staffed = base;
    staffed.staff = antwika::game::StoredStaff{
        .entries = {}, .ticksUntilDecay = 1};
    EXPECT_NE(base, staffed);

    auto employing = base;
    employing.employment = antwika::game::StoredEmployment{
        .jobs = {}, .ticksUntilDispatch = 1};
    EXPECT_NE(base, employing);

    auto other = base;
    other.kind = BuildingKind::House;
    EXPECT_NE(base, other);

    auto stocked = base;
    stocked.stock = {0, 0, 0};
    EXPECT_NE(base, stocked);

    auto risky = base;
    risky.risk = 0;
    EXPECT_NE(base, risky);

    auto cracking = base;
    cracking.collapseRisk = 9;
    EXPECT_NE(base, cracking);

    auto sickening = base;
    sickening.diseaseRisk = 9;
    EXPECT_NE(base, sickening);

    auto spawning = base;
    spawning.ticksUntilSpawn = 0;
    EXPECT_NE(base, spawning);

    auto draining = base;
    draining.ticksUntilDrain = 0;
    EXPECT_NE(base, draining);

    auto rising = base;
    rising.ticksUntilRisk = 0;
    EXPECT_NE(base, rising);

    auto trading = base;
    trading.selling = antwika::game::Resource::Pottery;
    EXPECT_NE(base, trading);

    auto alone = base;
    alone.walkers.clear();
    EXPECT_NE(base, alone);
}

TEST(SaveGameTest, OperatorEquals_ComparesTheBuildings)
{
    SaveGame base;
    base.buildings = {
        antwika::game::SavedBuilding{.at = {.x = 1, .y = 1}}};

    auto other = base;
    other.buildings.clear();

    const auto twin = base;
    EXPECT_EQ(base, twin);
    EXPECT_NE(base, other);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsABuildingsCoverage)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4},
        .kind = BuildingKind::House,
        .coverage = {kCoverageFull, 12}}};

    const auto encoded = saveGameToJson(save);

    ASSERT_TRUE(encoded.at("buildings").at(0).contains("coverage"));
    EXPECT_EQ(saveGameFromJson(encoded), save);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsTheGoodAMarketSellsNext)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 3, .y = 4},
        .kind = BuildingKind::Market,
        .selling = antwika::game::Resource::Pottery}};

    const auto encoded = saveGameToJson(save);

    ASSERT_EQ(encoded.at("buildings").at(0).at("selling"), "pottery");
    EXPECT_EQ(saveGameFromJson(encoded), save);
}

TEST(SaveGameTest, SaveGameToJson_WritesNoSellingForTheGoodItStartsOn)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{.at = {.x = 1, .y = 1}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("selling"));
    EXPECT_EQ(
        saveGameFromJson(encoded).buildings[0].selling,
        antwika::game::Resource::Food);
}

TEST(SaveGameTest, SaveGameFromJson_RefusesAGoodThisBuildDoesNotHave)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{.at = {.x = 1, .y = 1}}};

    auto encoded = saveGameToJson(save);
    encoded["buildings"][0]["selling"] = "marble";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameToJson_WritesNoUnreachedCoverage)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{.at = {.x = 1, .y = 1}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("coverage"));
    EXPECT_EQ(saveGameFromJson(encoded).buildings[0].coverage,
              (std::array<std::int32_t, kServiceCount>{}));
}

TEST(SaveGameTest, SaveGameFromJson_RejectsImpossibleCoverage)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 1, .y = 1}, .coverage = {kCoverageFull + 1, 0}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsAWalkerMidErrand)
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

TEST(SaveGameTest, SaveGameFromJson_RoundTripsAnErrandBoundNowhere)
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

TEST(SaveGameTest, SaveGameFromJson_RoundTripsAProducersCountdown)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4},
        .kind = BuildingKind::Farm,
        .ticksUntilOutput = 11}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.buildings, save.buildings);
}

TEST(SaveGameTest,
     SaveGameFromJson_LeavesABuildingThatNeverProducedWithoutACountdown)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("ticksUntilOutput"));
    EXPECT_FALSE(
        saveGameFromJson(encoded).buildings[0].ticksUntilOutput.has_value());
}

TEST(SaveGameTest,
     SaveGameFromJson_RejectsAnErrandWhoseDestinationIsNotABuildingInIt)
{
    SaveGame save;
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .errand = SavedErrand{.destination = 7U}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsACoverageArrayThatIsNotOnePerService)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{.at = {.x = 1, .y = 1}}};

    auto encoded = saveGameToJson(save);
    encoded["buildings"][0]["coverage"] = {1, 2, 3};

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest,
     SaveGameFromJson_RejectsAnErrandNamingAResourceThisBuildDoesNotHave)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 1, .y = 2}, .errand = SavedErrand{}}};

    auto encoded = saveGameToJson(save);
    encoded["walkers"][0]["errand"]["carrying"] = "amphorae";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest,
     SaveGameFromJson_ReadsAVersionThreeDocumentWrittenBeforeCoverage)
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

TEST(SaveGameTest, SaveGameOf_TakesEachBuildingsCoverageFromTheWorld)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};
    const PathIndex paths;

    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 2, .y = 2});
    world.add<Building>(house, Building{.kind = BuildingKind::House});
    antwika::game::setCoverage(
        world, house, antwika::game::Coverage{.ticksLeft = {4, 5}});
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{},
        GridExtent{.width = 8, .height = 8});

    ASSERT_EQ(save.buildings.size(), 1U);
    EXPECT_EQ(save.buildings[0].coverage,
              (std::array<std::int32_t, kServiceCount>{4, 5}));
}

TEST(SaveGameTest, OperatorEquals_SavedBuildingEqualityComparesTheCoverage)
{
    const antwika::game::SavedBuilding base{
        .at = {.x = 1, .y = 1}, .coverage = {1, 2}};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    for (std::size_t slot = 0; slot < kServiceCount; ++slot)
    {
        auto changed = base;
        changed.coverage[slot] += 1;

        EXPECT_NE(base, changed);
    }
}

TEST(SaveGameTest,
     SaveGameFromJson_RejectsAnErrandNamingALegThisBuildDoesNotHave)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{.at = {.x = 1, .y = 2}, .errand = SavedErrand{}}};

    auto encoded = saveGameToJson(save);
    encoded["walkers"][0]["errand"]["leg"] = "sideways";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameOf_TakesAnErrandAndACountdownFromARunningSession)
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

TEST(SaveGameTest, SaveGameOf_WritesAnErrandNamingNobodyWhenItsStoreIsGone)
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

TEST(SaveGameTest, SaveGameFromJson_RoundTripsAWalkerMidJourney)
{
    SaveGame save;
    save.walkers = {
        SavedWalker{
            .at = {.x = 1, .y = 2},
            .kind = WalkerKind::Migrant,
            .journey =
                SavedJourney{.towards = {.x = 4, .y = 4}, .house = 0U}},
        SavedWalker{
            .at = {.x = 3, .y = 2},
            .kind = WalkerKind::Migrant,
            .journey = SavedJourney{.towards = {.x = 0, .y = 2}}}};
    save.buildings = {
        antwika::game::SavedBuilding{
            .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.walkers, save.walkers);
    ASSERT_TRUE(loaded.walkers[1].journey.has_value());
    EXPECT_FALSE(loaded.walkers[1].journey->house.has_value());
}

TEST(SaveGameTest, SaveGameFromJson_LeavesAWalkerGoingNowhereWithoutAJourney)
{
    SaveGame save;
    save.walkers = {SavedWalker{.at = {.x = 1, .y = 2}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("walkers").at(0).contains("journey"));
    EXPECT_FALSE(
        saveGameFromJson(encoded).walkers[0].journey.has_value());
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAJourneyWhoseHouseIsNotABuildingInIt)
{
    SaveGame save;
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .journey = SavedJourney{.towards = {.x = 3, .y = 3}, .house = 7U}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameOf_WritesAJourneyAgainstTheHouseItNames)
{
    ::testing::NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 4, .y = 4});
    world.add<Building>(house, Building{.kind = BuildingKind::House});
    world.commit();

    const auto mover = world.create();
    world.add<Cell>(mover, Cell{.x = 1, .y = 1});
    world.add<Walker>(mover, Walker{.kind = WalkerKind::Migrant});
    world.add<antwika::game::Journey>(
        mover,
        antwika::game::Journey{
            .towards = Cell{.x = 4, .y = 4}, .house = house});

    const auto leaver = world.create();
    world.add<Cell>(leaver, Cell{.x = 2, .y = 1});
    world.add<Walker>(leaver, Walker{.kind = WalkerKind::Migrant});
    world.add<antwika::game::Journey>(
        leaver,
        antwika::game::Journey{.towards = Cell{.x = 0, .y = 1}});
    world.commit();

    const auto save = saveGameOf(
        world,
        paths,
        Camera{},
        GameState{},
        GridExtent{.width = 16, .height = 16});

    ASSERT_EQ(save.walkers.size(), 2U);
    ASSERT_TRUE(save.walkers[0].journey.has_value());
    EXPECT_EQ(save.walkers[0].journey->house, 0U);
    EXPECT_EQ(save.walkers[0].journey->towards, (Cell{.x = 4, .y = 4}));
    ASSERT_TRUE(save.walkers[1].journey.has_value());
    EXPECT_FALSE(save.walkers[1].journey->house.has_value());
}

TEST(SaveGameTest, SaveGameOf_WritesAJourneyNamingNobodyWhenItsHouseIsGone)
{
    ::testing::NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto mover = world.create();
    world.add<Cell>(mover, Cell{.x = 1, .y = 1});
    world.add<Walker>(mover, Walker{.kind = WalkerKind::Migrant});
    world.add<antwika::game::Journey>(
        mover,
        antwika::game::Journey{
            .towards = Cell{.x = 4, .y = 4},
            .house = static_cast<antwika::ecs::Entity>(99)});
    world.commit();

    const auto save = saveGameOf(
        world,
        paths,
        Camera{},
        GameState{},
        GridExtent{.width = 16, .height = 16});

    ASSERT_EQ(save.walkers.size(), 1U);
    ASSERT_TRUE(save.walkers[0].journey.has_value());
    EXPECT_FALSE(save.walkers[0].journey->house.has_value());
}

TEST(SaveGameTest, OperatorEquals_SavedJourneyEqualityComparesEveryField)
{
    const SavedJourney base{.towards = {.x = 1, .y = 2}, .house = 3U};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto elsewhere = base;
    elsewhere.towards = Cell{.x = 2, .y = 1};
    EXPECT_NE(base, elsewhere);

    auto leaving = base;
    leaving.house.reset();
    EXPECT_NE(base, leaving);
}

TEST(SaveGameTest, OperatorEquals_SavedWalkerEqualityComparesItsJourney)
{
    SavedWalker base{.at = {.x = 1, .y = 2}};
    base.journey = SavedJourney{};

    auto staying = base;
    staying.journey.reset();

    const auto twin = base;
    EXPECT_EQ(base, twin);
    EXPECT_NE(base, staying);
}

TEST(SaveGameTest, OperatorEquals_SavedErrandEqualityComparesEveryField)
{
    const SavedErrand base{
        .destination = 1U,
        .carrying = Resource::Clay,
        .leg = ErrandLeg::Returning};

    const auto twin = base;
    EXPECT_EQ(base, twin);

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

TEST(SaveGameTest, OperatorEquals_SavedWalkerEqualityComparesItsErrand)
{
    SavedWalker base{.at = {.x = 1, .y = 2}};
    base.errand = SavedErrand{};

    auto roaming = base;
    roaming.errand.reset();

    const auto twin = base;
    EXPECT_EQ(base, twin);
    EXPECT_NE(base, roaming);
}

TEST(SaveGameTest, OperatorEquals_SavedBuildingEqualityComparesItsCountdown)
{
    antwika::game::SavedBuilding base{.at = {.x = 1, .y = 2}};
    base.ticksUntilOutput = 4;

    auto idle = base;
    idle.ticksUntilOutput.reset();

    const auto twin = base;
    EXPECT_EQ(base, twin);
    EXPECT_NE(base, idle);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsACityOfSeveralBuildings)
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

TEST(SaveGameTest, SaveGameOf_TakesACityOfSeveralBuildingsFromARunningSession)
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

TEST(SaveGameTest, SaveGameFromJson_RoundTripsAHouseMidEvolution)
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

TEST(SaveGameTest, SaveGameToJson_WritesTheHousingLevelByName)
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

TEST(SaveGameTest, SaveGameFromJson_LeavesAHouseThatNeverGrewWithoutAHousehold)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("household"));
    EXPECT_FALSE(
        saveGameFromJson(encoded).buildings[0].household.has_value());
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAHousingLevelThisBuildDoesNotHave)
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

TEST(SaveGameTest, SaveGameFromJson_RejectsAHouseholdMissingOneOfItsMembers)
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

TEST(SaveGameTest,
     SaveGameFromJson_ReadsAVersionThreeDocumentWrittenBeforeHousing)
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

TEST(SaveGameTest, SaveGameOf_TakesEachHousesHouseholdFromTheWorld)
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

TEST(SaveGameTest, OperatorEquals_SavedBuildingEqualityComparesTheHousehold)
{
    const antwika::game::SavedBuilding base{
        .at = {.x = 1, .y = 1},
        .household = antwika::game::Household{
            .level = antwika::game::HousingLevel::Hovel}};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto homeless = base;
    homeless.household.reset();
    EXPECT_NE(base, homeless);

    auto grown = base;
    grown.household->level = antwika::game::HousingLevel::Cottage;
    EXPECT_NE(base, grown);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsTheTwoLabourLedgers)
{
    SaveGame save;
    save.buildings = {
        antwika::game::SavedBuilding{
            .at = {.x = 1, .y = 1},
            .kind = BuildingKind::Farm,
            .staff = antwika::game::StoredStaff{
                .entries = {antwika::game::StoredStaffEntry{
                    .house = 1, .count = 3}},
                .ticksUntilDecay = 5}},
        antwika::game::SavedBuilding{
            .at = {.x = 5, .y = 5},
            .kind = BuildingKind::House,
            .employment = antwika::game::StoredEmployment{
                .jobs = {antwika::game::StoredJob{
                    .workplace = 0, .count = 3}},
                .ticksUntilDispatch = 7}}};

    const auto encoded = saveGameToJson(save);

    EXPECT_TRUE(encoded.at("buildings").at(0).contains("staff"));
    EXPECT_FALSE(encoded.at("buildings").at(0).contains("employment"));
    EXPECT_TRUE(encoded.at("buildings").at(1).contains("employment"));
    EXPECT_EQ(saveGameFromJson(encoded), save);
}

TEST(SaveGameTest, SaveGameFromJson_AcceptsAndIgnoresTheLegacyEmployedCount)
{
    SaveGame save;
    save.buildings = {
        antwika::game::SavedBuilding{
            .at = {.x = 1, .y = 1}, .kind = BuildingKind::Farm}};

    auto encoded = saveGameToJson(save);
    encoded.at("buildings").at(0)["employed"] = 3;

    const auto loaded = saveGameFromJson(encoded);

    EXPECT_FALSE(loaded.buildings[0].staff.has_value());
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAStaffEntryNamingNoSuchBuilding)
{
    SaveGame save;
    save.buildings = {
        antwika::game::SavedBuilding{
            .at = {.x = 1, .y = 1},
            .kind = BuildingKind::Farm,
            .staff = antwika::game::StoredStaff{
                .entries = {antwika::game::StoredStaffEntry{
                    .house = 9, .count = 1}}}}};

    EXPECT_THROW(
        (void)saveGameFromJson(saveGameToJson(save)),
        antwika::game::SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAJobHoldingNamingNoSuchBuilding)
{
    SaveGame save;
    save.buildings = {
        antwika::game::SavedBuilding{
            .at = {.x = 1, .y = 1},
            .kind = BuildingKind::House,
            .employment = antwika::game::StoredEmployment{
                .jobs = {antwika::game::StoredJob{
                    .workplace = 9, .count = 1}}}}};

    EXPECT_THROW(
        (void)saveGameFromJson(saveGameToJson(save)),
        antwika::game::SaveFormatError);
}

TEST(SaveGameTest, SaveGameOf_TakesEachWorkplacesLedgerFromTheWorld)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};
    const PathIndex paths;

    const auto farm = world.create();
    world.add<Cell>(farm, Cell{.x = 2, .y = 2});
    world.add<Building>(farm, Building{.kind = BuildingKind::Farm});

    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 6, .y = 6});
    world.add<Building>(house, Building{.kind = BuildingKind::House});
    world.commit();

    antwika::game::Staff staff;
    staff.sources[0] =
        antwika::game::StaffEntry{.house = house, .count = 2};
    antwika::game::setStaff(world, farm, staff);
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{},
        GridExtent{.width = 8, .height = 8});

    ASSERT_EQ(save.buildings.size(), 2U);
    ASSERT_TRUE(save.buildings[0].staff.has_value());
    ASSERT_EQ(save.buildings[0].staff->entries.size(), 1U);
    EXPECT_EQ(save.buildings[0].staff->entries[0].house, 1U);
    EXPECT_EQ(save.buildings[0].staff->entries[0].count, 2);
    EXPECT_FALSE(save.buildings[1].staff.has_value());
}

TEST(SaveGameTest,
     SaveGameFromJson_ReadsAVersionThreeDocumentWrittenBeforeLabour)
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
             "ticksUntilRisk": 3,
             "household": {"level": "shack", "ticksUntilEvolve": 5,
                           "ticksUntilDevolve": 6, "population": 2}},
            {"x": 5, "y": 5, "kind": "farm", "stock": [0, 0, 0],
             "risk": 0, "ticksUntilSpawn": 1, "ticksUntilDrain": 2,
             "ticksUntilRisk": 3}
        ],
        "seed": 5
    })");

    const auto save = saveGameFromJson(document);

    ASSERT_EQ(save.buildings.size(), 2U);
    ASSERT_TRUE(save.buildings[0].household.has_value());
    EXPECT_EQ(save.buildings[0].household->population, 2);
    EXPECT_FALSE(save.buildings[0].staff.has_value());
    EXPECT_FALSE(save.buildings[1].staff.has_value());
}

TEST(SaveGameTest, SaveGameOf_DropsLedgerEntriesNamingNothingFromASave)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};
    const PathIndex paths;

    const auto farm = world.create();
    world.add<Cell>(farm, Cell{.x = 2, .y = 2});
    world.add<Building>(farm, Building{.kind = BuildingKind::Farm});

    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 6, .y = 6});
    world.add<Building>(house, Building{.kind = BuildingKind::House});

    const auto stray = world.create();
    world.add<Cell>(stray, Cell{.x = 1, .y = 1});
    world.commit();

    antwika::game::Staff staff;
    staff.sources[0] =
        antwika::game::StaffEntry{.house = house, .count = 0};
    staff.sources[1] =
        antwika::game::StaffEntry{.house = stray, .count = 2};
    antwika::game::setStaff(world, farm, staff);

    antwika::game::Employment employment;
    employment.jobs[0] =
        antwika::game::JobHolding{.workplace = farm, .count = 0};
    employment.jobs[1] =
        antwika::game::JobHolding{.workplace = stray, .count = 2};
    antwika::game::setEmployment(world, house, employment);
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{},
        GridExtent{.width = 8, .height = 8});

    ASSERT_TRUE(save.buildings[0].staff.has_value());
    EXPECT_TRUE(save.buildings[0].staff->entries.empty());
    ASSERT_TRUE(save.buildings[1].employment.has_value());
    EXPECT_TRUE(save.buildings[1].employment->jobs.empty());
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsAnEmptyLedger)
{
    SaveGame save;
    save.buildings = {
        antwika::game::SavedBuilding{
            .at = {.x = 1, .y = 1},
            .kind = BuildingKind::Farm,
            .staff = antwika::game::StoredStaff{
                .entries = {}, .ticksUntilDecay = 3},
            .employment = antwika::game::StoredEmployment{
                .jobs = {}, .ticksUntilDispatch = 4}}};

    EXPECT_EQ(saveGameFromJson(saveGameToJson(save)), save);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsARuinAndAFiremansCall)
{
    SaveGame save;
    save.ruins = {
        antwika::game::SavedRuin{
            .at = {.x = 4, .y = 4},
            .kind = BuildingKind::Farm,
            .state = antwika::game::RuinState::Burning,
            .ticksUntilOut = 123},
        antwika::game::SavedRuin{
            .at = {.x = 8, .y = 8},
            .kind = BuildingKind::House,
            .state = antwika::game::RuinState::Debris,
            .ticksUntilOut = 0}};
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .kind = WalkerKind::Fireman,
        .fireCall = 0U}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.ruins, save.ruins);
    EXPECT_EQ(loaded.walkers, save.walkers);
}

TEST(SaveGameTest, SaveGameFromJson_WritesNoRuinsMemberWhenNothingHasBurnt)
{
    const SaveGame nothingBurnt;
    const auto encoded = saveGameToJson(nothingBurnt);

    EXPECT_FALSE(encoded.contains("ruins"));
    EXPECT_TRUE(saveGameFromJson(encoded).ruins.empty());
}

TEST(SaveGameTest, SaveGameFromJson_RejectsARuinKindThisBuildDoesNotHave)
{
    SaveGame save;
    save.ruins = {antwika::game::SavedRuin{
        .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    auto encoded = saveGameToJson(save);
    encoded["ruins"][0]["kind"] = "castle";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsARuinStateThisBuildDoesNotHave)
{
    SaveGame save;
    save.ruins = {antwika::game::SavedRuin{
        .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    auto encoded = saveGameToJson(save);
    encoded["ruins"][0]["state"] = "ashes";

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_RejectsAFireCallWhoseRuinIsNotARuinInIt)
{
    SaveGame save;
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .kind = WalkerKind::Fireman,
        .fireCall = 7U}};

    const auto encoded = saveGameToJson(save);

    EXPECT_THROW((void)saveGameFromJson(encoded), SaveFormatError);
}

TEST(SaveGameTest, SaveGameFromJson_KeepsAFireCallToARuinAlreadyOut)
{
    SaveGame save;
    save.ruins = {antwika::game::SavedRuin{
        .at = {.x = 4, .y = 4},
        .kind = BuildingKind::House,
        .state = antwika::game::RuinState::Debris,
        .ticksUntilOut = 0}};
    save.walkers = {SavedWalker{
        .at = {.x = 1, .y = 2},
        .kind = WalkerKind::Fireman,
        .fireCall = 0U}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.walkers[0].fireCall, 0U);
}

TEST(SaveGameTest, SaveGameOf_TakesTheRuinsAndTheCallsFromARunningSession)
{
    ::testing::NiceMock<MockLogger> logger;
    antwika::ecs::World world(logger);
    const PathIndex paths;

    const auto fire = world.create();
    world.add<Cell>(fire, Cell{.x = 5, .y = 5});
    world.add<antwika::game::Ruin>(
        fire,
        antwika::game::Ruin{
            .kind = BuildingKind::Market, .ticksUntilOut = 77});

    const auto fireman = world.create();
    world.add<Cell>(fireman, Cell{.x = 1, .y = 1});
    world.add<Walker>(fireman, Walker{.kind = WalkerKind::Fireman});
    world.add<antwika::game::FireCall>(
        fireman, antwika::game::FireCall{.target = fire});
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{}, GridExtent{});

    ASSERT_EQ(save.ruins.size(), 1U);
    EXPECT_EQ(save.ruins[0].at, (Cell{.x = 5, .y = 5}));
    EXPECT_EQ(save.ruins[0].kind, BuildingKind::Market);
    EXPECT_EQ(
        save.ruins[0].state, antwika::game::RuinState::Burning);
    EXPECT_EQ(save.ruins[0].ticksUntilOut, 77);

    ASSERT_EQ(save.walkers.size(), 1U);
    EXPECT_EQ(save.walkers[0].fireCall, 0U);
}

TEST(SaveGameTest, SaveGameOf_DropsAFireCallWhoseRuinIsAlreadyGone)
{
    ::testing::NiceMock<MockLogger> logger;
    antwika::ecs::World world(logger);
    const PathIndex paths;

    const auto fire = world.create();
    world.add<Cell>(fire, Cell{.x = 5, .y = 5});
    world.add<antwika::game::Ruin>(
        fire, antwika::game::Ruin{.kind = BuildingKind::House});

    const auto fireman = world.create();
    world.add<Cell>(fireman, Cell{.x = 1, .y = 1});
    world.add<Walker>(fireman, Walker{.kind = WalkerKind::Fireman});
    world.add<antwika::game::FireCall>(
        fireman, antwika::game::FireCall{.target = fire});
    world.destroy(fire);
    world.commit();

    const auto save = saveGameOf(
        world, paths, Camera(), GameState{}, GridExtent{});

    EXPECT_TRUE(save.ruins.empty());
    ASSERT_EQ(save.walkers.size(), 1U);
    EXPECT_FALSE(save.walkers[0].fireCall.has_value());
}

TEST(SaveGameTest, OperatorEquals_SavedRuinEqualityComparesEveryField)
{
    const antwika::game::SavedRuin base{
        .at = {.x = 4, .y = 4},
        .kind = BuildingKind::Farm,
        .state = antwika::game::RuinState::Burning,
        .ticksUntilOut = 9};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto moved = base;
    moved.at = Cell{.x = 5, .y = 5};
    EXPECT_NE(base, moved);

    auto rekinded = base;
    rekinded.kind = BuildingKind::House;
    EXPECT_NE(base, rekinded);

    auto out = base;
    out.state = antwika::game::RuinState::Debris;
    EXPECT_NE(base, out);

    auto later = base;
    later.ticksUntilOut = 8;
    EXPECT_NE(base, later);
}

TEST(SaveGameTest, OperatorEquals_SavedWalkerEqualityComparesTheFireCall)
{
    const SavedWalker base{
        .at = {.x = 1, .y = 2}, .kind = WalkerKind::Fireman};

    auto called = base;
    called.fireCall = 0U;
    EXPECT_NE(base, called);
}

TEST(SaveGameTest, OperatorEquals_SaveEqualityComparesTheRuins)
{
    const auto base = populated();

    auto burnt = base;
    burnt.ruins.push_back(antwika::game::SavedRuin{});
    EXPECT_NE(base, burnt);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsACrackedBuildingsCollapseRisk)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4},
        .kind = BuildingKind::House,
        .collapseRisk = 17}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.buildings, save.buildings);
}

TEST(SaveGameTest, SaveGameFromJson_WritesNoCollapseRiskMemberAtNothing)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("collapseRisk"));
    EXPECT_EQ(saveGameFromJson(encoded).buildings[0].collapseRisk, 0);
}

TEST(SaveGameTest, SaveGameFromJson_RoundTripsASickBuildingsDiseaseRisk)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4},
        .kind = BuildingKind::House,
        .diseaseRisk = 23}};

    const auto loaded = saveGameFromJson(saveGameToJson(save));

    EXPECT_EQ(loaded.buildings, save.buildings);
}

TEST(SaveGameTest, SaveGameFromJson_WritesNoDiseaseRiskMemberAtNothing)
{
    SaveGame save;
    save.buildings = {antwika::game::SavedBuilding{
        .at = {.x = 4, .y = 4}, .kind = BuildingKind::House}};

    const auto encoded = saveGameToJson(save);

    EXPECT_FALSE(encoded.at("buildings").at(0).contains("diseaseRisk"));
    EXPECT_EQ(saveGameFromJson(encoded).buildings[0].diseaseRisk, 0);
}
