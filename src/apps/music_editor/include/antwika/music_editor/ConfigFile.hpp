#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/music_editor/MusicEditorConfig.hpp"

namespace antwika::music_editor
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-music-editor-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const MusicEditorConfig &config);

    [[nodiscard]] MusicEditorConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const MusicEditorConfig &config, std::ostream &out);

    [[nodiscard]] MusicEditorConfig readConfig(std::istream &in);

    [[nodiscard]] MusicEditorConfig loadConfigFileOrDefaults(
        const std::string &path);

}
