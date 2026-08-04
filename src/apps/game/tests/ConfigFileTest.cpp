#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/app/AssetPath.hpp>
#include <antwika/replay/SchemaVersion.hpp>

#include "ScratchDirectory.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/ConfigFile.hpp"
#include "antwika/game/ConfigFormatError.hpp"
#include "antwika/game/Tuning.hpp"

using antwika::game::tests::scratchDirectory;

using antwika::game::BuildingKind;
using antwika::game::ConfigFormatError;
using antwika::game::kBuildingKindCount;
using antwika::game::kConfigFormatVersion;
using antwika::game::kConfigMagic;
using antwika::game::loadConfigFileOrDefaults;
using antwika::game::readConfig;
using antwika::game::standardConfigMigrations;
using antwika::game::Tuning;
using antwika::game::tuningFromJson;
using antwika::game::tuningToJson;
using antwika::game::writeConfig;

namespace
{
    // Every member is off its default here.
    // A round trip that dropped one would land back on the default.
    // The comparison against this value is what would say so.
    [[nodiscard]] Tuning rebalanced()
    {
        Tuning tuning;
        tuning.startingMoney = 123;
        tuning.roadCost = 7;
        tuning.razeCost = 5;

        for (std::size_t index = 0; index < kBuildingKindCount; ++index)
        {
            tuning.buildingCosts[index] += 1;
        }

        tuning.riskPeriodTicks = 11;
        tuning.drainPeriodTicks = 12;
        tuning.mouthsPerServing = 3;
        tuning.spawnPeriodTicks = 13;
        tuning.burnDurationTicks = 14;
        tuning.settlerPeriodTicks = 15;
        tuning.evolvePeriodTicks = 16;
        tuning.devolvePeriodTicks = 17;
        tuning.productionPeriodTicks = 18;
        tuning.productionBatch = 19;
        tuning.labourPeriodTicks = 20;
        tuning.staffDecayPeriodTicks = 21;
        tuning.walkerLimit = 9;
        return tuning;
    }

    [[nodiscard]] std::string versionKey()
    {
        return std::string(antwika::replay::kSchemaVersionKey);
    }

    class ConfigFileTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            std::filesystem::create_directories(directory);
        }

        void TearDown() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }

        [[nodiscard]] std::string pathIn(const std::string &name) const
        {
            return (directory / name).string();
        }

        void writeText(const std::string &name, const std::string &text)
        {
            std::ofstream file(pathIn(name));
            file << text;
        }

        std::filesystem::path directory{scratchDirectory("config.")};
    };
} // namespace

TEST_F(ConfigFileTest, ADocumentStatesItsFormatAndItsVersion)
{
    const auto encoded = tuningToJson(Tuning{});

    EXPECT_EQ(encoded.at("magic").get<std::string>(), kConfigMagic);
    EXPECT_EQ(
        encoded.at(versionKey()).get<std::uint32_t>(),
        kConfigFormatVersion);
}

TEST_F(ConfigFileTest, ATuningRoundTripsThroughTheDocument)
{
    EXPECT_EQ(tuningFromJson(tuningToJson(rebalanced())), rebalanced());
    EXPECT_EQ(tuningFromJson(tuningToJson(Tuning{})), Tuning{});
}

// A config stating one number is a one-line rebalance.
// Not a restatement of every default it leaves alone.
TEST_F(ConfigFileTest, AMinimalDocumentIsTheShippedGame)
{
    nlohmann::json document;
    document["magic"] = std::string(kConfigMagic);

    EXPECT_EQ(tuningFromJson(document), Tuning{});
}

TEST_F(ConfigFileTest, APartialDocumentChangesOnlyWhatItStates)
{
    nlohmann::json document;
    document["magic"] = std::string(kConfigMagic);
    document["roadCost"] = 9;
    document["buildingCosts"]["well"] = 99;

    const auto tuning = tuningFromJson(document);

    Tuning expected;
    expected.roadCost = 9;
    expected.buildingCosts[antwika::game::buildingKindIndex(
        BuildingKind::Well)] = 99;

    EXPECT_EQ(tuning, expected);
}

TEST_F(ConfigFileTest, ATuningRoundTripsThroughAStream)
{
    std::stringstream stream;
    writeConfig(rebalanced(), stream);

    EXPECT_EQ(readConfig(stream), rebalanced());
}

TEST_F(ConfigFileTest, ATuningRoundTripsThroughAFile)
{
    std::stringstream stream;
    writeConfig(rebalanced(), stream);
    writeText("config.json", stream.str());

    EXPECT_EQ(loadConfigFileOrDefaults(pathIn("config.json")), rebalanced());
}

// A build nobody has rebalanced plays the game these sources define.
// That is a state, not a failure.
TEST_F(ConfigFileTest, AMissingFileIsTheShippedGame)
{
    EXPECT_EQ(
        loadConfigFileOrDefaults(pathIn("nothing-here.json")), Tuning{});
}

// The file beside the executable is the one main() reads.
// Pinned to the defaults, so shipping it changes nothing on its own.
// A rebalance is an edit to that file, and this test says where it is.
TEST_F(ConfigFileTest, TheShippedConfigStatesTheDefaults)
{
    EXPECT_EQ(
        loadConfigFileOrDefaults(antwika::app::assetPath("config.json")),
        Tuning{});
    EXPECT_TRUE(std::filesystem::exists(
        antwika::app::assetPath("config.json")));
}

TEST_F(ConfigFileTest, TextThatIsNotJsonIsRefused)
{
    writeText("config.json", "not json at all");

    EXPECT_THROW(
        (void)loadConfigFileOrDefaults(pathIn("config.json")),
        ConfigFormatError);
}

TEST_F(ConfigFileTest, ADocumentOfAnotherFormatIsRefused)
{
    auto document = tuningToJson(Tuning{});
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)tuningFromJson(document), ConfigFormatError);
}

// Read before anything is decoded.
// So a file from a build this one has never met is refused.
// Rather than read for happening to satisfy today's schema.
TEST_F(ConfigFileTest, ADocumentFromANewerBuildIsRefused)
{
    auto document = tuningToJson(Tuning{});
    document[versionKey()] = kConfigFormatVersion + 1;

    EXPECT_THROW((void)tuningFromJson(document), ConfigFormatError);
}

TEST_F(ConfigFileTest, ADocumentOfTheWrongShapeIsRefused)
{
    auto document = tuningToJson(Tuning{});
    document["startingMoney"] = "plenty";

    EXPECT_THROW((void)tuningFromJson(document), ConfigFormatError);
}

// A period of zero would never come due, or divide by zero.
// Refused rather than repaired, for ConfigFormatError's reason.
TEST_F(ConfigFileTest, AZeroPeriodIsRefused)
{
    auto document = tuningToJson(Tuning{});
    document["drainPeriodTicks"] = 0;

    EXPECT_THROW((void)tuningFromJson(document), ConfigFormatError);
}

TEST_F(ConfigFileTest, ANegativeCostIsRefused)
{
    auto document = tuningToJson(Tuning{});
    document["roadCost"] = -1;

    EXPECT_THROW((void)tuningFromJson(document), ConfigFormatError);
}

// A misspelt member would otherwise be a rebalance that never took.
TEST_F(ConfigFileTest, AnUnknownMemberIsRefused)
{
    auto document = tuningToJson(Tuning{});
    document["roadCosts"] = 9;

    EXPECT_THROW((void)tuningFromJson(document), ConfigFormatError);
}

TEST_F(ConfigFileTest, AnUnknownBuildingKindIsRefused)
{
    auto document = tuningToJson(Tuning{});
    document["buildingCosts"]["tower"] = 9;

    EXPECT_THROW((void)tuningFromJson(document), ConfigFormatError);
}

// There has only ever been one revision.
// The chain is here anyway.
// It is what refuses a document from a newer build.
TEST_F(ConfigFileTest, TheChainBringsDocumentsToTheCurrentVersion)
{
    EXPECT_EQ(
        standardConfigMigrations().currentVersion(),
        kConfigFormatVersion);
}
