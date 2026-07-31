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
    // A throwaway version 1 -> 2 step.
    // It stands in for the first real one this format ever needs.
    // Renaming a member is the usual breaking change.
    // It deliberately does not touch the version.
    // Stamping that is the chain's job.
    class RenameSeedToWorldSeed final : public IMigration
    {
    public:
        [[nodiscard]] std::uint32_t fromVersion() const noexcept override
        {
            return 1;
        }

        [[nodiscard]] std::uint32_t toVersion() const noexcept override
        {
            return 2;
        }

        [[nodiscard]] std::string_view name() const noexcept override
        {
            return "save-v1-to-v2-rename-seed";
        }

        void apply(nlohmann::json &document) const override
        {
            document["worldSeed"] = document.at("seed");
            document.erase("seed");
        }
    };

    MigrationChain chainToVersionTwo()
    {
        MigrationList migrations;
        migrations.push_back(std::make_shared<RenameSeedToWorldSeed>());
        return MigrationChain(std::move(migrations), 2);
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
TEST(SaveMigrationTest, CarriesAVersionOneDocumentUpToVersionTwo)
{
    auto document = saveGameToJson(populated());
    ASSERT_EQ(document.at(std::string(kSchemaVersionKey)).get<int>(), 1);

    chainToVersionTwo().migrate(document);

    EXPECT_FALSE(document.contains("seed"));
    EXPECT_EQ(document.at("worldSeed").get<std::uint64_t>(), 4242U);
    EXPECT_EQ(document.at(std::string(kSchemaVersionKey)).get<int>(), 2);
}

// A document written before the version member existed is version 1.
// So the same step applies to it.
// And it comes out stating where it got to.
TEST(SaveMigrationTest, CarriesAnUnversionedDocumentUpToVersionTwo)
{
    auto document = saveGameToJson(populated());
    document.erase(std::string(kSchemaVersionKey));

    chainToVersionTwo().migrate(document);

    EXPECT_EQ(document.at("worldSeed").get<std::uint64_t>(), 4242U);
    EXPECT_EQ(document.at(std::string(kSchemaVersionKey)).get<int>(), 2);
}

TEST(SaveMigrationTest, RefusesADocumentNewerThanTheChain)
{
    auto document = saveGameToJson(populated());
    document[std::string(kSchemaVersionKey)] = 3;

    EXPECT_THROW(chainToVersionTwo().migrate(document), SchemaVersionError);
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
