#include "antwika/task_worker/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>

namespace antwika::task_worker
{

    namespace
    {
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
    }

    ANTWIKA_CONFIG_FILE(
        "task_worker",
        TaskWorkerConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

}
