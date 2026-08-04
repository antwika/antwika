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

#include "antwika/life/ConfigFile.hpp"
#include "antwika/life/LifeConfig.hpp"

using antwika::config::ConfigFormatError;
using antwika::life::configFromJson;
using antwika::life::configToJson;
using antwika::life::kConfigFormatVersion;
using antwika::life::kConfigMagic;
using antwika::life::loadConfigFileOrDefaults;
using antwika::life::readConfig;
using antwika::life::standardConfigMigrations;
using antwika::life::writeConfig;
using antwika::life::LifeConfig;

namespace
{
    // Every member is off its default here.
    // A round trip that dropped one lands back on the default.
    // The comparisons below are what would say so.
    [[nodiscard]] LifeConfig retuned()
    {
        LifeConfig config;
        config.tickIntervalMs = 75;
        return config;
    }

    void expectEqual(
        const LifeConfig &decoded,
        const LifeConfig &expected)
    {
        EXPECT_EQ(decoded.tickIntervalMs, expected.tickIntervalMs);
    }

    void expectDefaults(const LifeConfig &decoded)
    {
        EXPECT_EQ(decoded.tickIntervalMs, LifeConfig{}.tickIntervalMs);
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
            / ("antwika-life-config." + std::to_string(::getpid()))};
    };
} // namespace

TEST_F(ConfigFileTest, ADocumentStatesItsFormatAndItsVersion)
{
    const auto encoded = configToJson(LifeConfig{});

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
    auto document = configToJson(LifeConfig{});
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// Read before anything is decoded.
// So a file from a build this one has never met is refused.
TEST_F(ConfigFileTest, ADocumentFromANewerBuildIsRefused)
{
    auto document = configToJson(LifeConfig{});
    document[versionKey()] = kConfigFormatVersion + 1;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

TEST_F(ConfigFileTest, ADocumentOfTheWrongShapeIsRefused)
{
    auto document = configToJson(LifeConfig{});
    document["tickIntervalMs"] = "plenty";

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// A value the field's meaning excludes is refused beside the parse.
TEST_F(ConfigFileTest, AValueBelowTheFloorIsRefused)
{
    auto document = configToJson(LifeConfig{});
    document["tickIntervalMs"] = 0;

    EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
}

// A misspelt member would otherwise be a change that never took.
TEST_F(ConfigFileTest, AnUnknownMemberIsRefused)
{
    auto document = configToJson(LifeConfig{});
    document["tickIntervalMsz"] = 9;

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
