#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/tower_defence/TowerDefenceConfig.hpp"

namespace antwika::tower_defence
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-tower-defence-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const TowerDefenceConfig &config);

    [[nodiscard]] TowerDefenceConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const TowerDefenceConfig &config, std::ostream &out);

    [[nodiscard]] TowerDefenceConfig readConfig(std::istream &in);

    [[nodiscard]] TowerDefenceConfig loadConfigFileOrDefaults(
        const std::string &path);

}
