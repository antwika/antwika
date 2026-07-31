#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/SchemaVersionError.hpp>

#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"

using antwika::game::Cell;
using antwika::game::kSaveFormatVersion;
using antwika::game::SaveFormatError;
using antwika::game::SaveGame;
using antwika::game::saveGameFromJson;
using antwika::game::saveGameToJson;
using antwika::game::standardSaveMigrations;
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
