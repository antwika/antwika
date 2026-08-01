#pragma once

#include <optional>
#include <span>
#include <string>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::sudoku
{

    /**
     * @brief How long a session runs when nobody says.
     *
     * **A puzzle has no end of its own**, so this is what keeps one
     * from running forever: an hour at the frame period the application
     * paces itself with.
     * A real backend reports the window closing and Escape, and either
     * ends a session long before this; the default `null` backend
     * reports neither, and every CI leg builds that one -- so an
     * uncapped run there is a run that never finishes.
     * `--max-ticks 0` asks for exactly that, for somebody sitting in
     * front of a real window with a hard puzzle.
     */
    inline constexpr antwika::time::Tick kDefaultMaxTicks = 90000;

    /**
     * @brief What this application was asked to play, and for how long.
     */
    struct SudokuOptions
    {
        /**
         * @brief The puzzle to open, if any.
         *
         * Nothing means the demo puzzle baked into PuzzleFile.hpp.
         */
        std::optional<std::string> puzzlePath{};

        /**
         * @brief How many ticks to run at most, or nothing for no cap.
         */
        std::optional<antwika::time::Tick> maxTicks{kDefaultMaxTicks};

        /**
         * @brief Compare two sets of options.
         * @param other The options to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const SudokuOptions &other) const
            = default;
    };

    /**
     * @brief The flags this application accepts on top of the replay
     * ones.
     *
     * Handed to antwika::app::runRecorded(), which parses them in the
     * same pass as its own -- parsing them in a pass of their own is
     * what once stopped apps/poker's `--tick-delay-ms` working, since
     * the other pass had already refused it.
     *
     * There is deliberately no `--locale` here, though the console
     * application this grew out of had one. A button is as wide as its
     * own label and a press is resolved against the layout those
     * produce, so the language is now something a recorded click
     * depends on -- and no recording carries a command line.
     * antwika/i18n/Translator.hpp states that rule; this is the
     * application that used to be its one exception.
     *
     * @return The table, for a main() to pass on.
     */
    [[nodiscard]] std::span<const antwika::cli::FlagSpec> sudokuFlags();

    /**
     * @brief Read this application's options out of a parsed command
     * line.
     *
     * A value that is not what a flag asks for is ignored and the
     * default kept, following apps/poker's `--tick-delay-ms`: a typo in
     * a tick count should not stop somebody playing.
     *
     * @param parsed A command line parsed against a table that included
     * sudokuFlags().
     * @return What was asked for.
     */
    [[nodiscard]] SudokuOptions sudokuOptionsFrom(
        const antwika::cli::CommandLine &parsed);

} // namespace antwika::sudoku
