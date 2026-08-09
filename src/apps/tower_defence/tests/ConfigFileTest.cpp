#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/tower_defence/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct TowerDefenceConfigTraits final
        {
            using Config = antwika::tower_defence::TowerDefenceConfig;

            static std::string_view magic()
            {
                return antwika::tower_defence::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::tower_defence::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::tower_defence::standardConfigMigrations();
            }

            static Config retuned()
            {
                Config config;
                config.startingLives = 3;
                config.framePeriodMs = 40;
                config.mobs = {
                    antwika::tower_defence::MobProfile{
                        .ticksPerCell = 5,
                        .health = 50,
                        .armour = 5,
                        .reward = 50},
                    antwika::tower_defence::MobProfile{
                        .ticksPerCell = 6,
                        .health = 60,
                        .armour = 6,
                        .reward = 60},
                    antwika::tower_defence::MobProfile{
                        .ticksPerCell = 7,
                        .health = 70,
                        .armour = 7,
                        .reward = 70},
                    antwika::tower_defence::MobProfile{
                        .ticksPerCell = 8,
                        .health = 80,
                        .armour = 8,
                        .reward = 80}};
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded.startingLives, expected.startingLives);
                EXPECT_EQ(
                    decoded.framePeriodMs, expected.framePeriodMs);
                EXPECT_EQ(decoded.mobs, expected.mobs);
            }

            static const char *floorMember()
            {
                return "startingLives";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::tower_defence::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::tower_defence::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::tower_defence::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::tower_defence::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::tower_defence::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-tower_defence-config";
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TowerDefence, ConfigFileContractTest, TowerDefenceConfigTraits);

}

namespace
{

    using antwika::tower_defence::MobProfile;

    TEST(MobProfileTest, OperatorEquals_ComparesEveryField)
    {
        const MobProfile base{
            .ticksPerCell = 2, .health = 6, .armour = 0, .reward = 10};

        EXPECT_EQ(
            base,
            (MobProfile{
                .ticksPerCell = 2,
                .health = 6,
                .armour = 0,
                .reward = 10}));
        EXPECT_NE(
            base,
            (MobProfile{
                .ticksPerCell = 3,
                .health = 6,
                .armour = 0,
                .reward = 10}));
        EXPECT_NE(
            base,
            (MobProfile{
                .ticksPerCell = 2,
                .health = 7,
                .armour = 0,
                .reward = 10}));
        EXPECT_NE(
            base,
            (MobProfile{
                .ticksPerCell = 2,
                .health = 6,
                .armour = 1,
                .reward = 10}));
        EXPECT_NE(
            base,
            (MobProfile{
                .ticksPerCell = 2,
                .health = 6,
                .armour = 0,
                .reward = 11}));
    }

}
