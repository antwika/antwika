#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/life/LifeConfig.hpp"

namespace antwika::life
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-life-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const LifeConfig &config);

    [[nodiscard]] LifeConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const LifeConfig &config, std::ostream &out);

    [[nodiscard]] LifeConfig readConfig(std::istream &in);

    [[nodiscard]] LifeConfig loadConfigFileOrDefaults(
        const std::string &path);

}
