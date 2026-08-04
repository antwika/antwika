#include "antwika/life/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>

namespace antwika::life
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["tickIntervalMs"] =
                wholeShape(1, std::numeric_limits<std::int32_t>::max());
        }

        void encodeMembers(const LifeConfig &config, nlohmann::json &out)
        {
            out["tickIntervalMs"] = config.tickIntervalMs;
        }

        LifeConfig decodeMembers(const nlohmann::json &document)
        {
            LifeConfig config;
            config.tickIntervalMs =
                memberOr(document, "tickIntervalMs", config.tickIntervalMs);
            return config;
        }
    } // namespace

    ANTWIKA_CONFIG_FILE(
        "life",
        LifeConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

} // namespace antwika::life
