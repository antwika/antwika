#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/atlas_editor/AtlasEditorConfig.hpp"

namespace antwika::atlas_editor
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-atlas-editor-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const AtlasEditorConfig &config);

    [[nodiscard]] AtlasEditorConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const AtlasEditorConfig &config, std::ostream &out);

    [[nodiscard]] AtlasEditorConfig readConfig(std::istream &in);

    [[nodiscard]] AtlasEditorConfig loadConfigFileOrDefaults(
        const std::string &path);

}
