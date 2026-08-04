#include "antwika/task_worker/ConfigFile.hpp"

#include <cstdint>
#include <limits>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::task_worker
{

    namespace
    {
        constexpr antwika::config::Format kFormat{
            .magic = kConfigMagic, .version = kConfigFormatVersion};

        nlohmann::json configSchema()
        {
            auto schema = antwika::config::documentSchema(
                kFormat, "antwika task_worker config document");
            schema["properties"]["workerCount"] =
                antwika::config::wholeShape(
                    1, std::numeric_limits<std::uint32_t>::max());
            schema["properties"]["tickIntervalMs"] =
                antwika::config::wholeShape(
                    1, std::numeric_limits<std::int32_t>::max());
            return schema;
        } // GCOVR_EXCL_LINE

        const nlohmann::json_schema::json_validator &configValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                configSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    MigrationChain standardConfigMigrations()
    {
        // Every branch left on the excluded line is the allocator's.
        // The list is empty, so no heap branch is taken here.
        // What is left is the throw edge of building it.
        return MigrationChain({}, kConfigFormatVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json configToJson(const TaskWorkerConfig &config)
    {
        auto encoded = antwika::config::newDocument(kFormat);
        encoded["workerCount"] = config.workerCount;
        encoded["tickIntervalMs"] = config.tickIntervalMs;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    TaskWorkerConfig configFromJson(const nlohmann::json &document)
    {
        using antwika::config::memberOr;

        const auto brought = antwika::config::migrated(
            document,
            standardConfigMigrations(),
            configValidator(),
            "antwika::task_worker: config JSON failed schema validation: ");

        TaskWorkerConfig config;
        config.workerCount =
            memberOr(brought, "workerCount", config.workerCount);
        config.tickIntervalMs =
            memberOr(brought, "tickIntervalMs", config.tickIntervalMs);
        return config;
    }

    void writeConfig(const TaskWorkerConfig &config, std::ostream &out)
    {
        antwika::config::writeConfig(configToJson(config), out);
    }

    TaskWorkerConfig readConfig(std::istream &in)
    {
        return configFromJson(antwika::config::parseConfig(in));
    }

    TaskWorkerConfig loadConfigFileOrDefaults(const std::string &path)
    {
        const auto document = antwika::config::parseConfigFile(path);

        // A file that is not there is an install nobody has tuned.
        // Which is a state rather than a failure.
        return document.has_value() ? configFromJson(*document)
                                    : TaskWorkerConfig{};
    }

} // namespace antwika::task_worker
