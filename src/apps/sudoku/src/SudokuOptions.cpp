#include "antwika/sudoku/SudokuOptions.hpp"

#include <array>
#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>

namespace antwika::sudoku
{

    namespace
    {
        constexpr std::string_view kPuzzleFlag = "--puzzle";
        constexpr std::string_view kMaxTicksFlag = "--max-ticks";

        constexpr std::array kFlags{
            antwika::cli::FlagSpec{
                .name = kPuzzleFlag,
                .valueName = "<path>",
                .help = "Play this puzzle; without it a well-known "
                        "easy one."},
            antwika::cli::FlagSpec{
                .name = kMaxTicksFlag,
                .valueName = "<n>",
                .help = "Give up after <n> ticks (default 90000; 0 "
                        "runs until the window is closed)."}};

        [[nodiscard]] std::optional<std::uint64_t> parseNumber(
            const std::string_view text)
        {
            std::uint64_t value = 0;
            const auto read = std::from_chars(
                text.data(), text.data() + text.size(), value);

            if (read.ec != std::errc{}
                || read.ptr != text.data() + text.size())
            {
                return std::nullopt;
            }

            return value;
        }
    } // namespace

    std::span<const antwika::cli::FlagSpec> sudokuFlags()
    {
        return kFlags;
    }

    SudokuOptions sudokuOptionsFrom(
        const antwika::cli::CommandLine &parsed)
    {
        SudokuOptions options;

        options.puzzlePath = parsed.value(kPuzzleFlag);

        if (const auto ticks = parsed.value(kMaxTicksFlag); ticks)
        {
            const auto given = parseNumber(*ticks);

            // Zero means no cap at all.
            // That is what somebody at a real window asks for.
            // Anything unreadable leaves the default in place.
            if (given.has_value())
            {
                options.maxTicks = *given == 0
                                       ? std::optional<
                                             antwika::time::Tick>{}
                                       : std::optional{*given};
            }
        }

        return options;
    } // GCOVR_EXCL_LINE

} // namespace antwika::sudoku
