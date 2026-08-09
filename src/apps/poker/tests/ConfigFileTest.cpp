#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/poker/ConfigFile.hpp"
#include "antwika/poker/RoomConfig.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct PokerConfigTraits final
        {
            using Config = antwika::poker::RoomConfig;

            static std::string_view magic()
            {
                return antwika::poker::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::poker::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::poker::standardConfigMigrations();
            }

            static Config retuned()
            {
                Config config;
                config.blinds.small = 25;
                config.blinds.big = 50;
                config.minimumBuyIn = 1000;
                config.handStrengths = {
                    10, 20, 30, 40, 50, 60, 70, 80, 90};
                config.thresholds = {
                    antwika::poker::AgentThresholds{
                        .raiseAt = 90, .callAt = 60},
                    antwika::poker::AgentThresholds{
                        .raiseAt = 75, .callAt = 45},
                    antwika::poker::AgentThresholds{
                        .raiseAt = 60, .callAt = 30}};
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(
                    decoded.blinds.small, expected.blinds.small);
                EXPECT_EQ(decoded.blinds.big, expected.blinds.big);
                EXPECT_EQ(
                    decoded.minimumBuyIn, expected.minimumBuyIn);
                EXPECT_EQ(
                    decoded.handStrengths, expected.handStrengths);
                EXPECT_EQ(decoded.thresholds, expected.thresholds);
            }

            static const char *floorMember()
            {
                return "smallBlind";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::poker::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::poker::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::poker::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::poker::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::poker::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-poker-config";
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Poker, ConfigFileContractTest, PokerConfigTraits);

}

namespace
{

    using antwika::config::ConfigFormatError;
    using antwika::poker::configFromJson;
    using antwika::poker::configToJson;
    using antwika::poker::RoomConfig;

    TEST(PokerConfigRulesTest, ConfigFromJson_RefusesABigBlindBelowTheSmall)
    {
        auto document = configToJson(RoomConfig{});
        document["smallBlind"] = 50;
        document["bigBlind"] = 25;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

    TEST(PokerConfigRulesTest, ConfigFromJson_RefusesABuyInBelowTheBigBlind)
    {
        auto document = configToJson(RoomConfig{});
        document["minimumBuyIn"] = 5;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

}

namespace
{

    using antwika::poker::kHandCategoryCount;

    TEST(PokerConfigRulesTest, ConfigFromJson_RefusesABackwardsStrengthTable)
    {
        auto document = configToJson(RoomConfig{});
        document["handStrengths"] = {90, 80, 30, 40, 50, 60, 70, 80, 90};

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

    TEST(PokerConfigRulesTest, ConfigFromJson_RefusesAWrongLengthTable)
    {
        auto document = configToJson(RoomConfig{});
        document["handStrengths"] = {20, 45, 62};

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

    TEST(PokerConfigRulesTest, ConfigFromJson_RefusesARatingPastTheScale)
    {
        auto document = configToJson(RoomConfig{});
        document["handStrengths"] = {
            20, 45, 62, 76, 85, 90, 95, 98, 101};

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

}

namespace
{

    TEST(PokerConfigRulesTest, ConfigFromJson_RefusesARaiseUnderTheCall)
    {
        auto document = configToJson(RoomConfig{});
        document["thresholds"][0]["raiseAt"] = 10;
        document["thresholds"][0]["callAt"] = 90;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

}

namespace
{

    using antwika::poker::AgentThresholds;

    TEST(AgentThresholdsTest, OperatorEquals_ComparesEveryField)
    {
        const AgentThresholds base{.raiseAt = 70, .callAt = 40};

        EXPECT_EQ(base, (AgentThresholds{.raiseAt = 70, .callAt = 40}));
        EXPECT_NE(base, (AgentThresholds{.raiseAt = 71, .callAt = 40}));
        EXPECT_NE(base, (AgentThresholds{.raiseAt = 70, .callAt = 41}));
    }

}
