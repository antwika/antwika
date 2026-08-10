#include "antwika/map_editor/ConfigFile.hpp"

#include <cstdint>
#include <string>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>

namespace antwika::map_editor
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["uiScale"] = wholeShape(2, 4);
        }

        void encodeMembers(
            const MapEditorConfig &config, nlohmann::json &out)
        {
            out["uiScale"] = config.uiScale;
        }

        MapEditorConfig decodeMembers(const nlohmann::json &document)
        {
            MapEditorConfig config;
            config.uiScale =
                memberOr(document, "uiScale", config.uiScale);
            return config;
        }
    }

    ANTWIKA_CONFIG_FILE(
        "map_editor",
        MapEditorConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

}
