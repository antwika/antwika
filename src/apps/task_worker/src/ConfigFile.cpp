#include "antwika/task_worker/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::task_worker
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["workerCount"] =
                wholeShape(
                    1, std::numeric_limits<std::uint32_t>::max());
            schema["properties"]["tickIntervalMs"] =
                wholeShape(1, std::numeric_limits<std::int32_t>::max());
        }

        void encodeMembers(const TaskWorkerConfig &config, nlohmann::json &out)
        {
            out["workerCount"] = config.workerCount;
            out["tickIntervalMs"] = config.tickIntervalMs;
        }

        TaskWorkerConfig decodeMembers(const nlohmann::json &document)
        {
            TaskWorkerConfig config;
            config.workerCount =
                memberOr(document, "workerCount", config.workerCount);
            config.tickIntervalMs =
                memberOr(document, "tickIntervalMs", config.tickIntervalMs);
            return config;
        }

        const FileFormat<TaskWorkerConfig> &fileFormat()
        {
            using AppFormat = FileFormat<TaskWorkerConfig>;

            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const AppFormat format(
                FormatSpec<TaskWorkerConfig>{
                    .format =
                        {.magic = kConfigMagic,
                         .version = kConfigFormatVersion},
                    .title = "antwika task_worker config document",
                    .whatFailed =
                        "antwika::task_worker: config JSON failed schema "
                        "validation: ",
                    .members = describeMembers,
                    .encode = encodeMembers,
                    .decode = decodeMembers,
                    .migrations = standardConfigMigrations}); // GCOVR_EXCL_LINE
            return format;
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
        return fileFormat().toJson(config);
    }

    TaskWorkerConfig configFromJson(const nlohmann::json &document)
    {
        return fileFormat().fromJson(document);
    }

    void writeConfig(const TaskWorkerConfig &config, std::ostream &out)
    {
        fileFormat().write(config, out);
    }

    TaskWorkerConfig readConfig(std::istream &in)
    {
        return fileFormat().read(in);
    }

    TaskWorkerConfig loadConfigFileOrDefaults(const std::string &path)
    {
        return fileFormat().loadFileOrDefaults(path);
    }

} // namespace antwika::task_worker
