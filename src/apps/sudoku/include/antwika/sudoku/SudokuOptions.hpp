#pragma once

#include <optional>
#include <span>
#include <string>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::sudoku
{

    inline constexpr antwika::time::Tick kDefaultMaxTicks = 90000;

    struct SudokuOptions final
    {
        std::optional<std::string> puzzlePath{};

        std::optional<antwika::time::Tick> maxTicks{kDefaultMaxTicks};

        [[nodiscard]] bool operator==(const SudokuOptions &other) const
            = default;
    };

    [[nodiscard]] std::span<const antwika::cli::FlagSpec> sudokuFlags();

    [[nodiscard]] SudokuOptions sudokuOptionsFrom(
        const antwika::cli::CommandLine &parsed);

}
