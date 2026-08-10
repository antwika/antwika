#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/GameConfig.hpp"

namespace antwika::game
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-game-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const GameConfig &config);

    [[nodiscard]] GameConfig configFromJson(const nlohmann::json &document);

    void writeConfig(const GameConfig &config, std::ostream &out);

    [[nodiscard]] GameConfig readConfig(std::istream &in);

    [[nodiscard]] GameConfig loadConfigFileOrDefaults(
        const std::string &path);

}
