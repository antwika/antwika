#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/map_editor/MapEditorConfig.hpp"

namespace antwika::map_editor
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-map-editor-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(
        const MapEditorConfig &config);

    [[nodiscard]] MapEditorConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const MapEditorConfig &config, std::ostream &out);

    [[nodiscard]] MapEditorConfig readConfig(std::istream &in);

    [[nodiscard]] MapEditorConfig loadConfigFileOrDefaults(
        const std::string &path);

}
