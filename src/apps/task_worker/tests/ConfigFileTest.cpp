#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/config/conformance/ConfigFileContractTest.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/task_worker/ConfigFile.hpp"

namespace antwika::config::conformance
{

    namespace
    {
        struct TaskWorkerConfigTraits final
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
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TaskWorker, ConfigFileContractTest, TaskWorkerConfigTraits);

}
