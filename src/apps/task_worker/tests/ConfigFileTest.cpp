#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/config/conformance/ConfigFileContract.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/task_worker/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        /**
         * @brief This application's config format, for the shared
         * contract suite.
         */
        struct TaskWorkerConfigTraits
        {
            using Config = antwika::task_worker::TaskWorkerConfig;

            static std::string_view magic()
            {
                return antwika::task_worker::kConfigMagic;
            }

            static std::uint32_t version()
            {
                return antwika::task_worker::kConfigFormatVersion;
            }

            static antwika::replay::MigrationChain migrations()
            {
                return antwika::task_worker::standardConfigMigrations();
            }

            // Every member is off its default here.
            // A dropped member lands on the default and fails below.
            static Config retuned()
            {
                Config config;
                config.workerCount = 5;
                config.tickIntervalMs = 100;
                return config;
            }

            static void expectEqual(
                const Config &decoded, const Config &expected)
            {
                EXPECT_EQ(decoded.workerCount, expected.workerCount);
                EXPECT_EQ(decoded.tickIntervalMs, expected.tickIntervalMs);
            }

            static const char *floorMember()
            {
                return "workerCount";
            }

            static nlohmann::json toJson(const Config &config)
            {
                return antwika::task_worker::configToJson(config);
            }

            static Config fromJson(const nlohmann::json &document)
            {
                return antwika::task_worker::configFromJson(document);
            }

            static void write(const Config &config, std::ostream &out)
            {
                antwika::task_worker::writeConfig(config, out);
            }

            static Config read(std::istream &in)
            {
                return antwika::task_worker::readConfig(in);
            }

            static Config loadFileOrDefaults(const std::string &path)
            {
                return antwika::task_worker::loadConfigFileOrDefaults(path);
            }

            static std::string scratchPrefix()
            {
                return "antwika-task_worker-config";
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TaskWorker, ConfigFileContract, TaskWorkerConfigTraits);

} // namespace antwika::config::conformance
