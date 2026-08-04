#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <unistd.h>

#include <nlohmann/json.hpp>

#include <antwika/app/AssetPath.hpp>
#include <antwika/config/ConfigFormatError.hpp>
#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/companion/ConfigFile.hpp"
#include "antwika/companion/Pet.hpp"

using antwika::config::ConfigFormatError;
using antwika::companion::configFromJson;
using antwika::companion::configToJson;
using antwika::companion::kConfigFormatVersion;
using antwika::companion::kConfigMagic;
using antwika::companion::loadConfigFileOrDefaults;
using antwika::companion::readConfig;
using antwika::companion::standardConfigMigrations;
using antwika::companion::writeConfig;
using antwika::companion::PetConfig;

namespace
{
    // Every member is off its default here.
    // A round trip that dropped one lands back on the default.
    // The comparisons below are what would say so.
    [[nodiscard]] PetConfig retuned()
    {
        PetConfig config;
        config.hungerPeriodTicks = 101;
        config.starvePeriodTicks = 102;
        config.funDecayPeriodTicks = 103;
        config.fretPeriodTicks = 104;
        config.recoverPeriodTicks = 105;
        config.restPeriodTicks = 106;
        config.sayingTicks = 107;
        config.chatterPeriodTicks = 108;
        config.drainHappyTicks = 109;
        config.drainContentTicks = 110;
        config.drainLowTicks = 111;
        config.drainMiserableTicks = 112;
        config.childTicks = 113;
        config.teenTicks = 114;
        config.adultTicks = 115;
        config.elderTicks = 116;
        config.hungerMax = 117;
        config.hungerThreshold = 118;
        config.feedRelief = 119;
        config.feedJoy = 120;
        config.funMax = 121;
        config.funStart = 122;
        config.playFun = 123;
        config.playHunger = 124;
        config.playEnergy = 125;
        config.playJoy = 126;
        config.tiredPercent = 55;
        config.happinessMax = 128;
        config.happinessStart = 129;
        config.happyBand = 130;
        config.contentBand = 131;
        config.disturbCost = 132;
        config.pesterCost = 133;
        return config;
    }

    void expectEqual(const PetConfig &decoded, const PetConfig &expected)
    {
        EXPECT_EQ(decoded.hungerPeriodTicks, expected.hungerPeriodTicks);
        EXPECT_EQ(decoded.starvePeriodTicks, expected.starvePeriodTicks);
        EXPECT_EQ(decoded.funDecayPeriodTicks, expected.funDecayPeriodTicks);
        EXPECT_EQ(decoded.fretPeriodTicks, expected.fretPeriodTicks);
        EXPECT_EQ(decoded.recoverPeriodTicks, expected.recoverPeriodTicks);
        EXPECT_EQ(decoded.restPeriodTicks, expected.restPeriodTicks);
        EXPECT_EQ(decoded.sayingTicks, expected.sayingTicks);
        EXPECT_EQ(decoded.chatterPeriodTicks, expected.chatterPeriodTicks);
        EXPECT_EQ(decoded.drainHappyTicks, expected.drainHappyTicks);
        EXPECT_EQ(decoded.drainContentTicks, expected.drainContentTicks);
        EXPECT_EQ(decoded.drainLowTicks, expected.drainLowTicks);
        EXPECT_EQ(decoded.drainMiserableTicks, expected.drainMiserableTicks);
        EXPECT_EQ(decoded.childTicks, expected.childTicks);
        EXPECT_EQ(decoded.teenTicks, expected.teenTicks);
        EXPECT_EQ(decoded.adultTicks, expected.adultTicks);
        EXPECT_EQ(decoded.elderTicks, expected.elderTicks);
        EXPECT_EQ(decoded.hungerMax, expected.hungerMax);
        EXPECT_EQ(decoded.hungerThreshold, expected.hungerThreshold);
        EXPECT_EQ(decoded.feedRelief, expected.feedRelief);
        EXPECT_EQ(decoded.feedJoy, expected.feedJoy);
        EXPECT_EQ(decoded.funMax, expected.funMax);
        EXPECT_EQ(decoded.funStart, expected.funStart);
        EXPECT_EQ(decoded.playFun, expected.playFun);
        EXPECT_EQ(decoded.playHunger, expected.playHunger);
        EXPECT_EQ(decoded.playEnergy, expected.playEnergy);
        EXPECT_EQ(decoded.playJoy, expected.playJoy);
        EXPECT_EQ(decoded.tiredPercent, expected.tiredPercent);
        EXPECT_EQ(decoded.happinessMax, expected.happinessMax);
        EXPECT_EQ(decoded.happinessStart, expected.happinessStart);
        EXPECT_EQ(decoded.happyBand, expected.happyBand);
        EXPECT_EQ(decoded.contentBand, expected.contentBand);
        EXPECT_EQ(decoded.disturbCost, expected.disturbCost);
        EXPECT_EQ(decoded.pesterCost, expected.pesterCost);
    }

    void expectDefaults(const PetConfig &decoded)
    {
        expectEqual(decoded, PetConfig{});
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

        // Named per process, for game's ScratchDirectory.hpp's reason.
        // CTest runs every case as its own process.
        // A never-before-seen path cannot be mid-removal already.
        std::filesystem::path directory{
            std::filesystem::temp_directory_path()
            / ("antwika-companion-config." + std::to_string(::getpid()))};
    };
} // namespace

TEST_F(ConfigFileTest, ADocumentStatesItsFormatAndItsVersion)
{
    const auto encoded = configToJson(PetConfig{});

    EXPECT_EQ(encoded.at("magic").get<std::string>(), kConfigMagic);
    EXPECT_EQ(
        encoded.at(versionKey()).get<std::uint32_t>(),
        kConfigFormatVersion);
}

TEST_F(ConfigFileTest, AConfigRoundTripsThroughTheDocument)
{
    expectEqual(configFromJson(configToJson(retuned())), retuned());
}

// A config stating one number is a one-line change.
// Not a restatement of every default it leaves alone.
TEST_F(ConfigFileTest, AMinimalDocumentIsTheShippedApplication)
{
    nlohmann::json document;
    document["magic"] = std::string(kConfigMagic);

    expectDefaults(configFromJson(document));
}

TEST_F(ConfigFileTest, AConfigRoundTripsThroughAStream)
{
    std::stringstream stream;
    writeConfig(retuned(), stream);

    expectEqual(readConfig(stream), retuned());
}

TEST_F(ConfigFileTest, AConfigRoundTripsThroughAFile)
{
    std::stringstream stream;
    writeConfig(retuned(), stream);
    writeText("config.json", stream.str());

    expectEqual(loadConfigFileOrDefaults(pathIn("config.json")), retuned());
}

// An install nobody has tuned plays the shipped defaults.
// That is a state, not a failure.
TEST_F(ConfigFileTest, AMissingFileIsTheShippedApplication)
{
    expectDefaults(loadConfigFileOrDefaults(pathIn("nothing.json")));
}

// The file beside the executable is the one main() reads.
// Pinned to the defaults, so shipping it changes nothing on its own.
TEST_F(ConfigFileTest, TheShippedConfigStatesTheDefaults)
{
    expectDefaults(
        loadConfigFileOrDefaults(antwika::app::assetPath("config.json")));
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
    auto document = configToJson(PetConfig{});
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// Read before anything is decoded.
// So a file from a build this one has never met is refused.
TEST_F(ConfigFileTest, ADocumentFromANewerBuildIsRefused)
{
    auto document = configToJson(PetConfig{});
    document[versionKey()] = kConfigFormatVersion + 1;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

TEST_F(ConfigFileTest, ADocumentOfTheWrongShapeIsRefused)
{
    auto document = configToJson(PetConfig{});
    document["hungerPeriodTicks"] = "plenty";

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// A value the field's meaning excludes is refused beside the parse.
TEST_F(ConfigFileTest, AValueBelowTheFloorIsRefused)
{
    auto document = configToJson(PetConfig{});
    document["hungerPeriodTicks"] = 0;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// A misspelt member would otherwise be a change that never took.
TEST_F(ConfigFileTest, AnUnknownMemberIsRefused)
{
    auto document = configToJson(PetConfig{});
    document["hungerPeriodTicksz"] = 9;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// There has only ever been one revision; the chain is here anyway.
// It is what refuses a document from a newer build.
TEST_F(ConfigFileTest, TheChainBringsDocumentsToTheCurrentVersion)
{
    EXPECT_EQ(
        standardConfigMigrations().currentVersion(),
        kConfigFormatVersion);
}
