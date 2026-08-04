#include "antwika/music_editor/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>

namespace antwika::music_editor
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

        void encodeMembers(const MusicEditorConfig &config, nlohmann::json &out)
        {
            out["tickIntervalMs"] = config.tickIntervalMs;
        }

        MusicEditorConfig decodeMembers(const nlohmann::json &document)
        {
            MusicEditorConfig config;
            config.tickIntervalMs =
                memberOr(document, "tickIntervalMs", config.tickIntervalMs);
            return config;
        }
    } // namespace

    ANTWIKA_CONFIG_FILE(
        "music_editor",
        MusicEditorConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

} // namespace antwika::music_editor
