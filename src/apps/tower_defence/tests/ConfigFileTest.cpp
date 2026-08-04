#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/config/conformance/ConfigFileContract.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/tower_defence/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        /**
         * @brief This application's config format, for the shared
         * contract suite.
         */
        struct TowerDefenceConfigTraits
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

            // Every member is off its default here.
            // A dropped member lands on the default and fails below.
            static Config retuned()
            {
                Config config;
                config.startingLives = 3;
                config.framePeriodMs = 40;
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded.startingLives, expected.startingLives);
                EXPECT_EQ(decoded.framePeriodMs, expected.framePeriodMs);
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
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TowerDefence, ConfigFileContract, TowerDefenceConfigTraits);

} // namespace antwika::config::conformance
