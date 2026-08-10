#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/poker/RoomConfig.hpp"

namespace antwika::poker
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-poker-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const RoomConfig &config);

    [[nodiscard]] RoomConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const RoomConfig &config, std::ostream &out);

    [[nodiscard]] RoomConfig readConfig(std::istream &in);

    [[nodiscard]] RoomConfig loadConfigFileOrDefaults(
        const std::string &path);

}
