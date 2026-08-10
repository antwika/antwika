#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/sudoku/SudokuConfig.hpp"

namespace antwika::sudoku
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-sudoku-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const SudokuConfig &config);

    [[nodiscard]] SudokuConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const SudokuConfig &config, std::ostream &out);

    [[nodiscard]] SudokuConfig readConfig(std::istream &in);

    [[nodiscard]] SudokuConfig loadConfigFileOrDefaults(
        const std::string &path);

}
