#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"

using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::GameState;
using antwika::game::GameSummary;
using antwika::game::GridExtent;
using antwika::game::kSaveFormatVersion;
using antwika::replay::kSchemaVersionKey;
using antwika::game::pathIndexOf;
using antwika::game::Point;
using antwika::game::SaveFormatError;
using antwika::game::SaveGame;
using antwika::game::saveGameFromJson;
using antwika::game::saveGameOf;
using antwika::game::saveGameToJson;
using antwika::game::WalkerView;

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
            WalkerView{.at = {.x = 1, .y = 0}, .facing = Direction::South},
            WalkerView{.at = {.x = 0, .y = 0}, .facing = Direction::West},
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
        WalkerView{.at = {.x = 0, .y = 0}, .facing = Direction::North},
        WalkerView{.at = {.x = 1, .y = 0}, .facing = Direction::East},
        WalkerView{.at = {.x = 2, .y = 0}, .facing = Direction::South},
        WalkerView{.at = {.x = 3, .y = 0}, .facing = Direction::West},
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
        WalkerView{.at = {.x = 2, .y = 3}, .facing = Direction::North}};

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
    encoded["buildings"] = nlohmann::json::array();

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

TEST(SaveGameTest, TakesASaveFromASummary)
{
    const auto expected = populated();
    const GameSummary summary{
        .state = expected.state,
        .paths = expected.paths,
        .walkers = expected.walkers,
        .camera = expected.camera,
    };

    EXPECT_EQ(saveGameOf(summary, expected.extent, expected.seed),
              expected);
}

TEST(SaveGameTest, DefaultsTheSeedWhenASummaryHasNoneToGive)
{
    const GameSummary summary;

    EXPECT_EQ(saveGameOf(summary, GridExtent{.width = 4, .height = 4}).seed,
              0U);
}

TEST(SaveGameTest, RebuildsThePathIndex)
{
    const auto index = pathIndexOf(populated());

    EXPECT_EQ(index.size(), 3U);
    EXPECT_TRUE(index.has(Cell{.x = 1, .y = 1}));
    EXPECT_FALSE(index.has(Cell{.x = 9, .y = 9}));
}
