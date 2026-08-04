#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/config/conformance/ConfigFileContract.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/ConfigFile.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/GameConfig.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        /**
         * @brief This application's config format, for the shared
         * contract suite.
         */
        struct GameConfigTraits
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

            // Every member is off its default here.
            // A dropped member lands on the default and fails below.
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
                config.settlerPeriodTicks = 15;
                config.evolvePeriodTicks = 16;
                config.devolvePeriodTicks = 17;
                config.productionPeriodTicks = 18;
                config.productionBatch = 19;
                config.labourPeriodTicks = 20;
                config.staffDecayPeriodTicks = 21;
                config.walkerLimit = 9;
                config.sustaining = {false, true, true};
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
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Game, ConfigFileContract, GameConfigTraits);

} // namespace antwika::config::conformance

namespace
{

    using antwika::config::ConfigFormatError;
    using antwika::game::BuildingKind;
    using antwika::game::configFromJson;
    using antwika::game::configToJson;
    using antwika::game::GameConfig;
    using antwika::game::kConfigMagic;

    TEST(GameConfigRulesTest, APartialDocumentChangesOnlyWhatItStates)
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

    TEST(GameConfigRulesTest, ANegativeCostIsRefused)
    {
        auto document = configToJson(GameConfig{});
        document["roadCost"] = -1;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

    TEST(GameConfigRulesTest, AnUnknownBuildingKindIsRefused)
    {
        auto document = configToJson(GameConfig{});
        document["buildingCosts"]["tower"] = 9;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

} // namespace

namespace
{

    // A household that needs nothing can never empty.
    // That turns the population rule off rather than tuning it.
    TEST(GameConfigRulesTest, AConfigNamingNoStapleIsRefused)
    {
        auto document = configToJson(GameConfig{});
        document["sustaining"] = {false, false, false};

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

} // namespace
