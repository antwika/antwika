#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/config/conformance/ConfigFileContract.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/life/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        /**
         * @brief This application's config format, for the shared
         * contract suite.
         */
        struct LifeConfigTraits
        {
            using Config = antwika::life::LifeConfig;

            static std::string_view magic()
            {
                return antwika::life::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::life::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::life::standardConfigMigrations();
            }

            // Every member is off its default here.
            // A dropped member lands on the default and fails below.
            static Config retuned()
            {
                Config config;
                config.tickIntervalMs = 75;
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded.tickIntervalMs, expected.tickIntervalMs);
            }

            static const char *floorMember()
            {
                return "tickIntervalMs";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::life::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::life::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::life::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::life::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::life::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-life-config";
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Life, ConfigFileContract, LifeConfigTraits);

} // namespace antwika::config::conformance
