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
            schema["properties"]["fullscreen"]["type"] = "boolean";
            schema["properties"]["keys"]["type"] = "object";
            schema["properties"]["keys"]["additionalProperties"]
                  ["type"] = "string";
        }

        void encodeMembers(
            const MapEditorConfig &config, nlohmann::json &out)
        {
            out["uiScale"] = config.uiScale;
            out["fullscreen"] = config.fullscreen;
            out["keys"] = config.keys;
        }

        MapEditorConfig decodeMembers(const nlohmann::json &document)
        {
            MapEditorConfig config;
            config.uiScale =
                memberOr(document, "uiScale", config.uiScale);
            config.fullscreen =
                memberOr(document, "fullscreen", config.fullscreen);
            config.keys = memberOr(document, "keys", config.keys);
            return config;
        } // GCOVR_EXCL_LINE
    }

    ANTWIKA_CONFIG_FILE(
        "map_editor",
        MapEditorConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

}
