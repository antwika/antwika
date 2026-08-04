#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/app/AssetPath.hpp>
#include <antwika/config/ConfigFormatError.hpp>
#include <antwika/replay/SchemaVersion.hpp>

#include "ScratchDirectory.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/ConfigFile.hpp"
#include "antwika/game/GameConfig.hpp"

using antwika::game::tests::scratchDirectory;

using antwika::config::ConfigFormatError;
using antwika::game::BuildingKind;
using antwika::game::configFromJson;
using antwika::game::configToJson;
using antwika::game::GameConfig;
using antwika::game::kBuildingKindCount;
using antwika::game::kConfigFormatVersion;
using antwika::game::kConfigMagic;
using antwika::game::loadConfigFileOrDefaults;
using antwika::game::readConfig;
using antwika::game::standardConfigMigrations;
using antwika::game::writeConfig;

namespace
{
    // Every member is off its default here.
    // A round trip that dropped one would land back on the default.
    // The comparison against this value is what would say so.
    [[nodiscard]] GameConfig rebalanced()
    {
        GameConfig config;
        config.startingMoney = 123;
        config.roadCost = 7;
        config.razeCost = 5;

        for (std::size_t index = 0; index < kBuildingKindCount; ++index)
        {
            config.buildingCosts[index] += 1;
        }

        config.riskPeriodTicks = 11;
        config.drainPeriodTicks = 12;
        config.mouthsPerServing = 3;
        config.spawnPeriodTicks = 13;
        config.burnDurationTicks = 14;
        config.settlerPeriodTicks = 15;
        config.evolvePeriodTicks = 16;
        config.devolvePeriodTicks = 17;
        config.productionPeriodTicks = 18;
        config.productionBatch = 19;
        config.labourPeriodTicks = 20;
        config.staffDecayPeriodTicks = 21;
        config.walkerLimit = 9;
        return config;
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
    const auto encoded = configToJson(GameConfig{});

    EXPECT_EQ(encoded.at("magic").get<std::string>(), kConfigMagic);
    EXPECT_EQ(
        encoded.at(versionKey()).get<std::uint32_t>(),
        kConfigFormatVersion);
}

TEST_F(ConfigFileTest, ATuningRoundTripsThroughTheDocument)
{
    EXPECT_EQ(configFromJson(configToJson(rebalanced())), rebalanced());
    EXPECT_EQ(configFromJson(configToJson(GameConfig{})), GameConfig{});
}

// A config stating one number is a one-line rebalance.
// Not a restatement of every default it leaves alone.
TEST_F(ConfigFileTest, AMinimalDocumentIsTheShippedGame)
{
    nlohmann::json document;
    document["magic"] = std::string(kConfigMagic);

    EXPECT_EQ(configFromJson(document), GameConfig{});
}

TEST_F(ConfigFileTest, APartialDocumentChangesOnlyWhatItStates)
{
    nlohmann::json document;
    document["magic"] = std::string(kConfigMagic);
    document["roadCost"] = 9;
    document["buildingCosts"]["well"] = 99;

    const auto config = configFromJson(document);

    GameConfig expected;
    expected.roadCost = 9;
    expected.buildingCosts[antwika::game::buildingKindIndex(
        BuildingKind::Well)] = 99;

    EXPECT_EQ(config, expected);
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
        loadConfigFileOrDefaults(pathIn("nothing-here.json")), GameConfig{});
}

// The file beside the executable is the one main() reads.
// Pinned to the defaults, so shipping it changes nothing on its own.
// A rebalance is an edit to that file, and this test says where it is.
TEST_F(ConfigFileTest, TheShippedConfigStatesTheDefaults)
{
    EXPECT_EQ(
        loadConfigFileOrDefaults(antwika::app::assetPath("config.json")),
        GameConfig{});
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
    auto document = configToJson(GameConfig{});
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// Read before anything is decoded.
// So a file from a build this one has never met is refused.
// Rather than read for happening to satisfy today's schema.
TEST_F(ConfigFileTest, ADocumentFromANewerBuildIsRefused)
{
    auto document = configToJson(GameConfig{});
    document[versionKey()] = kConfigFormatVersion + 1;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

TEST_F(ConfigFileTest, ADocumentOfTheWrongShapeIsRefused)
{
    auto document = configToJson(GameConfig{});
    document["startingMoney"] = "plenty";

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// A period of zero would never come due, or divide by zero.
// Refused rather than repaired, for ConfigFormatError's reason.
TEST_F(ConfigFileTest, AZeroPeriodIsRefused)
{
    auto document = configToJson(GameConfig{});
    document["drainPeriodTicks"] = 0;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

TEST_F(ConfigFileTest, ANegativeCostIsRefused)
{
    auto document = configToJson(GameConfig{});
    document["roadCost"] = -1;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// A misspelt member would otherwise be a rebalance that never took.
TEST_F(ConfigFileTest, AnUnknownMemberIsRefused)
{
    auto document = configToJson(GameConfig{});
    document["roadCosts"] = 9;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

TEST_F(ConfigFileTest, AnUnknownBuildingKindIsRefused)
{
    auto document = configToJson(GameConfig{});
    document["buildingCosts"]["tower"] = 9;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
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
