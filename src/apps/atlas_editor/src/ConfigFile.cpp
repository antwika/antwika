#include "antwika/atlas_editor/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>

namespace antwika::atlas_editor
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["framePeriodMs"] =
                wholeShape(1, std::numeric_limits<std::int32_t>::max());
        }

        void encodeMembers(const AtlasEditorConfig &config, nlohmann::json &out)
        {
            out["framePeriodMs"] = config.framePeriodMs;
        }

        AtlasEditorConfig decodeMembers(const nlohmann::json &document)
        {
            AtlasEditorConfig config;
            config.framePeriodMs =
                memberOr(document, "framePeriodMs", config.framePeriodMs);
            return config;
        }
    } // namespace

    ANTWIKA_CONFIG_FILE(
        "atlas_editor",
        AtlasEditorConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

} // namespace antwika::atlas_editor
