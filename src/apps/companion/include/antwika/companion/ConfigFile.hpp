#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-companion-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const PetConfig &config);

    [[nodiscard]] PetConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const PetConfig &config, std::ostream &out);

    [[nodiscard]] PetConfig readConfig(std::istream &in);

    [[nodiscard]] PetConfig loadConfigFileOrDefaults(
        const std::string &path);

}
