#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/ConfigFormatError.hpp>
#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/ConfigFile.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/GameConfig.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct GameConfigTraits final
        {
            using Config = antwika::game::GameConfig;

            static std::string_view magic()
            {
                return antwika::game::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::game::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::game::standardConfigMigrations();
            }

            static Config retuned()
            {
                Config config;
                config.startingMoney = 123;
                config.roadCost = 7;
                config.razeCost = 5;

                for (std::size_t index = 0;
                     index < antwika::game::kBuildingKindCount;
                     ++index)
                {
                    config.buildingCosts[index] += 1;
                }

                config.riskPeriodTicks = 11;
                config.drainPeriodTicks = 12;
                config.mouthsPerServing = 3;
                config.spawnPeriodTicks = 13;
                config.burnDurationTicks = 14;
                config.migrantPeriodTicks = 22;
                config.evolvePeriodTicks = 16;
                config.devolvePeriodTicks = 17;
                config.productionPeriodTicks = 18;
                config.productionBatch = 19;
                config.labourPeriodTicks = 20;
                config.staffDecayPeriodTicks = 21;
                config.walkerLimit = 9;
                config.sustaining = {false, true, true};
                config.atlases = {
                    .byKind = {"a.png", "b.png", "c.png"},
                    .walker = "d.png"};
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded, expected);
            }

            static const char *floorMember()
            {
                return "drainPeriodTicks";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::game::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::game::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::game::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::game::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::game::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-game-config";
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Game, ConfigFileContractTest, GameConfigTraits);

}

namespace
{

    using antwika::config::ConfigFormatError;
    using antwika::game::BuildingKind;
    using antwika::game::configFromJson;
    using antwika::game::configToJson;
    using antwika::game::GameConfig;
    using antwika::game::kConfigMagic;

    TEST(GameConfigRulesTest, ConfigFromJson_ChangesOnlyWhatItStates)
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

    TEST(GameConfigRulesTest, ConfigFromJson_ANegativeCostIsRefused)
    {
        auto document = configToJson(GameConfig{});
        document["roadCost"] = -1;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

    TEST(GameConfigRulesTest, ConfigFromJson_AnUnknownBuildingKindIsRefused)
    {
        auto document = configToJson(GameConfig{});
        document["buildingCosts"]["tower"] = 9;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

}

namespace
{

    TEST(GameConfigRulesTest, ConfigFromJson_AConfigNamingNoStapleIsRefused)
    {
        auto document = configToJson(GameConfig{});
        document["sustaining"] = {false, false, false};

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

}

TEST(GameConfigFileTest, ConfigFromJson_ReadsTheSheetsTheDocumentNames)
{
    auto document =
        antwika::game::configToJson(antwika::game::GameConfig{});
    document["atlases"] = {"one.png", "two.png", "three.png"};

    const auto config = antwika::game::configFromJson(document);

    EXPECT_EQ(
        config.atlases.byKind,
        (std::array<std::string, antwika::game::kAtlasKindCount>{
            "one.png", "two.png", "three.png"}));
}

TEST(GameConfigFileTest, ConfigFromJson_ReadsTheSheetTheWalkersMarchOn)
{
    auto document =
        antwika::game::configToJson(antwika::game::GameConfig{});
    document["walkerAtlas"] = "marching.png";

    EXPECT_EQ(
        antwika::game::configFromJson(document).atlases.walker,
        "marching.png");
}

TEST(GameConfigFileTest, ConfigFromJson_FallsBackToTheShippedWalkerSheet)
{
    auto document =
        antwika::game::configToJson(antwika::game::GameConfig{});
    document.erase("walkerAtlas");

    EXPECT_EQ(
        antwika::game::configFromJson(document).atlases.walker,
        antwika::game::defaultAtlases().walker);
}

TEST(GameConfigFileTest, ConfigFromJson_RefusesAWalkerSheetWithNoName)
{
    auto document =
        antwika::game::configToJson(antwika::game::GameConfig{});
    document["walkerAtlas"] = "";

    EXPECT_THROW(
        [[maybe_unused]] const auto config =
            antwika::game::configFromJson(document),
        antwika::config::ConfigFormatError);
}

TEST(GameConfigFileTest, ConfigFromJson_FallsBackToTheShippedSheets)
{
    auto document =
        antwika::game::configToJson(antwika::game::GameConfig{});
    document.erase("atlases");

    EXPECT_EQ(
        antwika::game::configFromJson(document).atlases,
        antwika::game::defaultAtlases());
}

TEST(GameConfigFileTest, ConfigFromJson_RefusesASheetWithNoName)
{
    auto document =
        antwika::game::configToJson(antwika::game::GameConfig{});
    document["atlases"] = {"", "two.png", "three.png"};

    EXPECT_THROW(
        [[maybe_unused]] const auto config =
            antwika::game::configFromJson(document),
        antwika::config::ConfigFormatError);
}

TEST(GameConfigFileTest, OperatorEquals_ComparesTheSheetsAConfigNames)
{
    const antwika::game::GameConfig config;

    EXPECT_EQ(config, antwika::game::GameConfig{});

    auto renamed = config;
    renamed.atlases.byKind[0] = "elsewhere.png";

    EXPECT_NE(config, renamed);

    auto marching = config;
    marching.atlases.walker = "elsewhere.png";

    EXPECT_NE(config, marching);
}
