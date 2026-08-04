#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/config/conformance/ConfigFileContract.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/poker/ConfigFile.hpp"
#include "antwika/poker/RoomConfig.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        /**
         * @brief This application's config format, for the shared
         * contract suite.
         */
        struct PokerConfigTraits
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

            // Every member is off its default here.
            // A dropped member lands on the default and fails below.
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
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Poker, ConfigFileContract, PokerConfigTraits);

} // namespace antwika::config::conformance

namespace
{

    using antwika::config::ConfigFormatError;
    using antwika::poker::configFromJson;
    using antwika::poker::configToJson;
    using antwika::poker::RoomConfig;

    // The schema checks each number alone.
    // These two rules are between numbers.
    // A table nobody could sit at is refused outright.
    TEST(PokerConfigRulesTest, ABigBlindBelowTheSmallIsRefused)
    {
        auto document = configToJson(RoomConfig{});
        document["smallBlind"] = 50;
        document["bigBlind"] = 25;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

    TEST(PokerConfigRulesTest, ABuyInBelowTheBigBlindIsRefused)
    {
        auto document = configToJson(RoomConfig{});
        document["minimumBuyIn"] = 5;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

} // namespace

namespace
{

    using antwika::poker::kHandCategoryCount;

    // Weakest first is the table's whole meaning.
    // A straight rated under a pair was written backwards.
    TEST(PokerConfigRulesTest, ABackwardsStrengthTableIsRefused)
    {
        auto document = configToJson(RoomConfig{});
        document["handStrengths"] = {90, 80, 30, 40, 50, 60, 70, 80, 90};

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

    // Nine categories exist, so nine ratings are the only legal count.
    TEST(PokerConfigRulesTest, AStrengthTableOfTheWrongLengthIsRefused)
    {
        auto document = configToJson(RoomConfig{});
        document["handStrengths"] = {20, 45, 62};

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

    TEST(PokerConfigRulesTest, ARatingPastTheScaleIsRefused)
    {
        auto document = configToJson(RoomConfig{});
        document["handStrengths"] = {
            20, 45, 62, 76, 85, 90, 95, 98, 101};

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

} // namespace

namespace
{

    // An agent that raises hands it would not call is one nobody meant.
    TEST(PokerConfigRulesTest, ARaiseThresholdUnderTheCallIsRefused)
    {
        auto document = configToJson(RoomConfig{});
        document["thresholds"][0]["raiseAt"] = 10;
        document["thresholds"][0]["callAt"] = 90;

        EXPECT_THROW((void)configFromJson(document), ConfigFormatError);
    }

} // namespace

namespace
{

    using antwika::poker::AgentThresholds;

    // A defaulted operator== is one comparison per member.
    // A test that varies one member proves only that member is in it.
    TEST(AgentThresholdsTest, EqualityComparesEveryField)
    {
        const AgentThresholds base{.raiseAt = 70, .callAt = 40};

        EXPECT_EQ(base, (AgentThresholds{.raiseAt = 70, .callAt = 40}));
        EXPECT_NE(base, (AgentThresholds{.raiseAt = 71, .callAt = 40}));
        EXPECT_NE(base, (AgentThresholds{.raiseAt = 70, .callAt = 41}));
    }

} // namespace
