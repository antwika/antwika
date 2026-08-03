#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/SchemaVersionError.hpp>

#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"
#include "SaveMigrationV2ToV3.hpp"
#include "SaveMigrationV3ToV4.hpp"

using antwika::game::BuildingKind;
using antwika::game::Cell;
using antwika::game::DropRiskServices;
using antwika::game::kSaveFormatVersion;
using antwika::game::RenameToServices;
using antwika::game::SaveFormatError;
using antwika::game::SaveGame;
using antwika::game::saveGameFromJson;
using antwika::game::saveGameToJson;
using antwika::game::standardSaveMigrations;
using antwika::game::WalkerKind;
using antwika::replay::IMigration;
using antwika::replay::kSchemaVersionKey;
using antwika::replay::MigrationChain;
using antwika::replay::MigrationList;
using antwika::replay::SchemaVersionError;

namespace
{
    // A throwaway step off whatever version this build writes.
    // It stands in for the next real one this format ever needs.
    // Renaming a member is the usual breaking change.
    // It deliberately does not touch the version.
    // Stamping that is the chain's job.
    // Stated relative to kSaveFormatVersion rather than as a number.
    // So a real bump does not turn this proof into a failure.
    class RenameSeedToWorldSeed final : public IMigration
    {
    public:
        [[nodiscard]] std::uint32_t fromVersion() const noexcept override
        {
            return kSaveFormatVersion;
        }

        [[nodiscard]] std::uint32_t toVersion() const noexcept override
        {
            return kSaveFormatVersion + 1;
        }

        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "save-rename-seed";
        }

        void apply(nlohmann::json &document) const override
        {
            document["worldSeed"] = document.at("seed");
            document.erase("seed");
        }
    };

    MigrationChain chainToTheNextVersion()
    {
        MigrationList migrations;
        migrations.push_back(std::make_shared<RenameSeedToWorldSeed>());
        return MigrationChain(
            std::move(migrations), kSaveFormatVersion + 1);
    }

    SaveGame populated()
    {
        SaveGame save;
        save.paths = {Cell{.x = 1, .y = 1}};
        save.seed = 4242;
        return save;
    }
} // namespace

// The proof that the seam is a seam.
// A document this build writes today is carried forward.
// The chain doing it knows one more version than this build does.
TEST(SaveMigrationTest, CarriesADocumentUpToTheNextVersion)
{
    auto document = saveGameToJson(populated());
    ASSERT_EQ(
        document.at(std::string(kSchemaVersionKey)).get<std::uint32_t>(),
        kSaveFormatVersion);

    chainToTheNextVersion().migrate(document);

    EXPECT_FALSE(document.contains("seed"));
    EXPECT_EQ(document.at("worldSeed").get<std::uint64_t>(), 4242U);
    EXPECT_EQ(
        document.at(std::string(kSchemaVersionKey)).get<std::uint32_t>(),
        kSaveFormatVersion + 1);
}

// A document written before the version member existed is version 1.
// So the same step applies to it.
// And it comes out stating where it got to.
// A document with no version member is version 1.
// Which is what every file written before the member says.
// The standard chain is what carries one forward now.
TEST(SaveMigrationTest, CarriesAnUnversionedDocumentUpToTheCurrentVersion)
{
    auto document = saveGameToJson(populated());
    document.erase(std::string(kSchemaVersionKey));
    document.erase("buildings");

    standardSaveMigrations().migrate(document);

    EXPECT_TRUE(document.at("buildings").is_array());
    EXPECT_TRUE(document.at("buildings").empty());
    EXPECT_EQ(
        document.at(std::string(kSchemaVersionKey)).get<std::uint32_t>(),
        kSaveFormatVersion);
}

TEST(SaveMigrationTest, RefusesADocumentNewerThanTheChain)
{
    auto document = saveGameToJson(populated());
    document[std::string(kSchemaVersionKey)] = kSaveFormatVersion + 2;

    EXPECT_THROW(
        chainToTheNextVersion().migrate(document), SchemaVersionError);
}

TEST(SaveMigrationTest, TheStandardChainIsAtTheCurrentSaveVersion)
{
    EXPECT_EQ(standardSaveMigrations().currentVersion(),
              kSaveFormatVersion);
}

TEST(SaveMigrationTest, TheStandardChainLeavesACurrentDocumentAlone)
{
    const auto original = saveGameToJson(populated());
    auto document = original;

    standardSaveMigrations().migrate(document);

    EXPECT_EQ(document, original);
}

// The chain throws SchemaVersionError, which narrows ReplayFormatError.
// A caller loading a save catches SaveFormatError and nothing else.
// So the loader translates it at the boundary.
// An interface that promised one type must not leak another's.
TEST(SaveMigrationTest, TheLoaderReportsAChainFailureAsASaveFailure)
{
    auto document = saveGameToJson(populated());
    document[std::string(kSchemaVersionKey)] = kSaveFormatVersion + 1;

    EXPECT_THROW((void)saveGameFromJson(document), SaveFormatError);
}

// And it says which version it found.
// That is the only thing a person can act on.
TEST(SaveMigrationTest, TheTranslatedMessageKeepsTheChainsWording)
{
    auto document = saveGameToJson(populated());
    document[std::string(kSchemaVersionKey)] = 99;

    try
    {
        (void)saveGameFromJson(document);
        FAIL() << "a newer save should have been refused";
    }
    catch (const SaveFormatError &error)
    {
        EXPECT_NE(std::string(error.what()).find("99"),
                  std::string::npos);
    }
}

// **A hand-written version 2 document.**
// Which is what step 4 of docs/schema-versioning.md requires.
// One this build produced is a version 3 file renumbered.
// And that proves nothing about what version 2 wrote.
namespace
{
    constexpr const char *kVersionTwoDocument = R"({
        "magic": "antwika-game-save",
        "version": 2,
        "state": {"ticksProcessed": 12, "score": 3},
        "extent": {"width": 16, "height": 16},
        "camera": {"panX": 8, "panY": 4, "zoomLevel": 2},
        "paths": [{"x": 1, "y": 1}, {"x": 1, "y": 2}],
        "walkers": [
            {
                "x": 1, "y": 1,
                "facing": "north",
                "kind": "food",
                "carried": 40,
                "stepsUntilHome": 9,
                "ticksUntilStep": 1,
                "home": 0
            },
            {
                "x": 1, "y": 2,
                "facing": "west",
                "kind": "water",
                "carried": 70,
                "stepsUntilHome": 3,
                "ticksUntilStep": 0
            },
            {
                "x": 1, "y": 2,
                "facing": "east",
                "kind": "architect",
                "carried": 0,
                "stepsUntilHome": 2,
                "ticksUntilStep": 0
            }
        ],
        "buildings": [
            {
                "x": 4, "y": 4,
                "kind": "food_source",
                "stock": [55, 66],
                "risk": 7,
                "ticksUntilSpawn": 6,
                "ticksUntilDrain": 5,
                "ticksUntilRisk": 4,
                "walker": 0
            },
            {
                "x": 7, "y": 7,
                "kind": "water_source",
                "stock": [10, 20],
                "risk": 0,
                "ticksUntilSpawn": 1,
                "ticksUntilDrain": 2,
                "ticksUntilRisk": 3,
                "walker": null
            },
            {
                "x": 9, "y": 9,
                "kind": "architect_post",
                "stock": [1, 2],
                "risk": 0,
                "ticksUntilSpawn": 1,
                "ticksUntilDrain": 2,
                "ticksUntilRisk": 3
            }
        ],
        "seed": 99
    })";

    [[nodiscard]] nlohmann::json versionTwoDocument()
    {
        return nlohmann::json::parse(kVersionTwoDocument);
    }
} // namespace

// A save the previous version wrote has to still load.
// And this is what says so.
TEST(SaveMigrationTest, LoadsAHandWrittenVersionTwoSave)
{
    const auto save = saveGameFromJson(versionTwoDocument());

    EXPECT_EQ(save.state.ticksProcessed, 12U);
    EXPECT_EQ(save.seed, 99U);
    ASSERT_EQ(save.paths.size(), 2U);
    ASSERT_EQ(save.walkers.size(), 3U);
    ASSERT_EQ(save.buildings.size(), 3U);
}

TEST(SaveMigrationTest, RenamesEveryKindVersionTwoCalledSomethingElse)
{
    const auto save = saveGameFromJson(versionTwoDocument());

    EXPECT_EQ(save.buildings[0].kind, BuildingKind::Farm);
    EXPECT_EQ(save.buildings[1].kind, BuildingKind::Well);
    EXPECT_EQ(save.buildings[2].kind, BuildingKind::EngineerPost);

    EXPECT_EQ(save.walkers[0].kind, WalkerKind::MarketSeller);
    EXPECT_EQ(save.walkers[1].kind, WalkerKind::WaterCarrier);
    EXPECT_EQ(save.walkers[2].kind, WalkerKind::Engineer);
}

// Water was never a good.
// So a version 2 shelf of it reads as none of what nobody had made.
TEST(SaveMigrationTest, ReadsAVersionTwoStockAsFoodAndNoGoodsAtAll)
{
    const auto save = saveGameFromJson(versionTwoDocument());

    EXPECT_EQ(
        save.buildings[0].stock,
        (std::array<std::int32_t, antwika::game::kResourceCount>{
            55, 0, 0}));
    EXPECT_EQ(
        save.buildings[1].stock,
        (std::array<std::int32_t, antwika::game::kResourceCount>{
            10, 0, 0}));
}

// The one index version 2 wrote becomes a list of one.
// An explicit null and an absent member both mean nobody.
TEST(SaveMigrationTest, WrapsAVersionTwoWalkerLinkInAList)
{
    const auto save = saveGameFromJson(versionTwoDocument());

    EXPECT_EQ(
        save.buildings[0].walkers, (std::vector<std::size_t>{0U}));
    EXPECT_TRUE(save.buildings[1].walkers.empty());
    EXPECT_TRUE(save.buildings[2].walkers.empty());
}

// A version 2 document is what the chain reads.
// So it must not need this build to have written it.
TEST(SaveMigrationTest, CarriesAHandWrittenVersionTwoDocumentToCurrent)
{
    auto document = versionTwoDocument();

    standardSaveMigrations().migrate(document);

    EXPECT_EQ(
        document.at(std::string(kSchemaVersionKey)).get<std::uint32_t>(),
        kSaveFormatVersion);
    EXPECT_FALSE(document.at("buildings").at(0).contains("walker"));
}

// The step is driven on its own here.
// A document a decode would refuse is what a hand edit produces.
// A migration has to leave one alone rather than throw.
// Anything else would escape a caller catching SaveFormatError.
TEST(SaveMigrationTest, LeavesAMalformedVersionTwoEntryUntouched)
{
    const RenameToServices step;

    nlohmann::json document;
    document["buildings"] = nlohmann::json::array();
    document["buildings"].push_back(nlohmann::json::object());
    document["buildings"].push_back({{"kind", 7}, {"stock", 3}});
    document["buildings"].push_back(
        {{"kind", "food_source"}, {"stock", nlohmann::json::array({1})}});
    document["walkers"] = nlohmann::json::array();
    document["walkers"].push_back({{"kind", "nothing_it_knows"}});

    step.apply(document);

    EXPECT_TRUE(document.at("buildings").at(0).empty());
    EXPECT_EQ(document.at("buildings").at(1).at("kind").get<int>(), 7);
    EXPECT_EQ(
        document.at("buildings").at(2).at("stock"),
        nlohmann::json::array({1}));
    EXPECT_EQ(
        document.at("buildings").at(2).at("kind").get<std::string>(),
        "farm");
    EXPECT_EQ(
        document.at("walkers").at(0).at("kind").get<std::string>(),
        "nothing_it_knows");
}

// A document holding neither array is one nothing has to be done to.
TEST(SaveMigrationTest, LeavesADocumentWithNeitherArrayAlone)
{
    const RenameToServices step;

    nlohmann::json document;
    document["seed"] = 1;
    document["buildings"] = 5;
    document["walkers"] = "not an array";

    const auto before = document;
    step.apply(document);

    EXPECT_EQ(document, before);
}

// A document holding neither member is one there is nothing to do to.
// A hand-written file is allowed to be that empty.
TEST(SaveMigrationTest, LeavesADocumentNamingNeitherArrayAlone)
{
    const RenameToServices step;

    nlohmann::json document;
    document["seed"] = 1;

    const auto before = document;
    step.apply(document);

    EXPECT_EQ(document, before);
}

TEST(SaveMigrationTest, TheVersionTwoStepIsOneStepAndSaysWhichOne)
{
    const RenameToServices step;

    EXPECT_EQ(step.fromVersion(), 2U);
    EXPECT_EQ(step.toVersion(), 3U);
}

// A version 3 file counted coverage for four services.
// Water and health kept their slots; the truncation drops the rest.
TEST(SaveMigrationTest, TruncatesAVersionThreeCoverageToTwoServices)
{
    nlohmann::json document;
    document[std::string(kSchemaVersionKey)] = 3;
    document["buildings"] = nlohmann::json::array();
    document["buildings"].push_back(
        {{"coverage", nlohmann::json::array({40, 30, 20, 10})}});
    document["buildings"].push_back(nlohmann::json::object());

    standardSaveMigrations().migrate(document);

    EXPECT_EQ(
        document.at(std::string(kSchemaVersionKey)).get<std::uint32_t>(),
        kSaveFormatVersion);
    EXPECT_EQ(
        document.at("buildings").at(0).at("coverage"),
        nlohmann::json::array({40, 30}));
    EXPECT_FALSE(document.at("buildings").at(1).contains("coverage"));
}

// The step is driven on its own here, on the version 2 step's terms.
// A malformed document is left alone rather than thrown over.
TEST(SaveMigrationTest, TheVersionThreeStepLeavesAMalformedEntryAlone)
{
    const DropRiskServices step;

    nlohmann::json document;
    document["buildings"] = nlohmann::json::array();
    document["buildings"].push_back({{"coverage", "not an array"}});
    document["buildings"].push_back(
        {{"coverage", nlohmann::json::array({1, 2})}});

    const auto before = document;
    step.apply(document);

    EXPECT_EQ(document, before);
}

// A document with no buildings at all is one with nothing to do.
TEST(SaveMigrationTest, TheVersionThreeStepLeavesNoBuildingsAlone)
{
    const DropRiskServices step;

    nlohmann::json missing;
    missing["seed"] = 1;

    nlohmann::json wrong;
    wrong["buildings"] = "not an array";

    auto beforeMissing = missing;
    auto beforeWrong = wrong;
    step.apply(missing);
    step.apply(wrong);

    EXPECT_EQ(missing, beforeMissing);
    EXPECT_EQ(wrong, beforeWrong);
}

TEST(SaveMigrationTest, TheVersionThreeStepIsOneStepAndSaysWhichOne)
{
    const DropRiskServices step;

    EXPECT_EQ(step.fromVersion(), 3U);
    EXPECT_EQ(step.toVersion(), 4U);
}
