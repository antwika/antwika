#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/companion/ConfigFile.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct CompanionConfigTraits final
        {
            using Config = antwika::companion::PetConfig;

            static std::string_view magic()
            {
                return antwika::companion::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::companion::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::companion::standardConfigMigrations();
            }

            static Config retuned()
            {
                Config config;
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

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(
                    decoded.hungerPeriodTicks,
                    expected.hungerPeriodTicks);
                EXPECT_EQ(
                    decoded.starvePeriodTicks,
                    expected.starvePeriodTicks);
                EXPECT_EQ(
                    decoded.funDecayPeriodTicks,
                    expected.funDecayPeriodTicks);
                EXPECT_EQ(decoded.fretPeriodTicks, expected.fretPeriodTicks);
                EXPECT_EQ(
                    decoded.recoverPeriodTicks,
                    expected.recoverPeriodTicks);
                EXPECT_EQ(decoded.restPeriodTicks, expected.restPeriodTicks);
                EXPECT_EQ(decoded.sayingTicks, expected.sayingTicks);
                EXPECT_EQ(
                    decoded.chatterPeriodTicks,
                    expected.chatterPeriodTicks);
                EXPECT_EQ(decoded.drainHappyTicks, expected.drainHappyTicks);
                EXPECT_EQ(
                    decoded.drainContentTicks,
                    expected.drainContentTicks);
                EXPECT_EQ(decoded.drainLowTicks, expected.drainLowTicks);
                EXPECT_EQ(
                    decoded.drainMiserableTicks,
                    expected.drainMiserableTicks);
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

            static const char *floorMember()
            {
                return "hungerPeriodTicks";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::companion::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::companion::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::companion::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::companion::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::companion::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-companion-config";
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Companion, ConfigFileContractTest, CompanionConfigTraits);

}
