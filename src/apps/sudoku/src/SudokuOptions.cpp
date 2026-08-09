#include "antwika/sudoku/SudokuOptions.hpp"

#include <array>
#include <string_view>

#include <antwika/app/MaxTicks.hpp>

namespace antwika::sudoku
{

    namespace
    {
        constexpr std::string_view kPuzzleFlag = "--puzzle";

        constexpr std::array kFlags{
            antwika::cli::FlagSpec{
                .name = kPuzzleFlag,
                .valueName = "<path>",
                .help = "Play this puzzle; without it a well-known "
                        "easy one."},
            antwika::cli::FlagSpec{
                .name = antwika::app::kMaxTicksFlag,
                .valueName = "<n>",
                .help = "Give up after <n> ticks (default 90000; 0 "
                        "runs until the window is closed)."}};
    }

    std::span<const antwika::cli::FlagSpec> sudokuFlags()
    {
        return kFlags;
    }

    SudokuOptions sudokuOptionsFrom(
        const antwika::cli::CommandLine &parsed)
    {
        SudokuOptions options;

        options.puzzlePath = parsed.value(kPuzzleFlag);

        options.maxTicks = antwika::app::maxTicksOf(
            parsed.value(antwika::app::kMaxTicksFlag),
            options.maxTicks);

        return options;
    } // GCOVR_EXCL_LINE

}
