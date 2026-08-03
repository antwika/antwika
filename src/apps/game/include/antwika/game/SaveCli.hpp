#pragma once

#include <optional>
#include <span>
#include <string>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>

namespace antwika::game
{

    /**
     * @brief The `--save`/`--load` file paths a run was invoked with.
     */
    struct SaveCliOptions
    {
        /**
         * @brief Where to write the session's state when the run ends,
         * if `--save <path>` was given.
         */
        std::optional<std::string> savePath;

        /**
         * @brief Where to read the session's starting state from, if
         * `--load <path>` was given.
         *
         * Unset starts an empty grid, which is what a session with
         * nothing loaded into it means.
         */
        std::optional<std::string> loadPath;
    };

    /**
     * @brief The save flags this app accepts, beside the replay ones.
     * @return The table, to concatenate with
     * antwika::replay::replayCliFlags() and parse in one pass.
     *
     * Parsing twice would make each pass refuse the other's flags, which
     * is why this is a table rather than a parser -- the same reason
     * replayCliFlags() is one.
     */
    [[nodiscard]] std::span<const antwika::cli::FlagSpec> saveCliFlags();

    /**
     * @brief Pick the save options out of an already-parsed command line.
     * @param parsed A command line parsed against a table that included
     * saveCliFlags().
     * @return The save options it holds.
     */
    [[nodiscard]] SaveCliOptions saveCliOptionsFrom(
        const antwika::cli::CommandLine &parsed);

    /**
     * @brief Refuse `--record` beside `--load`.
     * @param options The save options the run was invoked with.
     * @param recording Whether `--record` was given too.
     * @throws antwika::cli::CommandLineError for the pair.
     *
     * A recording keeps only input, and a loaded city is not input:
     * it reaches the session directly, through no event.
     * So a `--record` run started from a save would replay against an
     * empty grid, and every recorded click would resolve against a
     * different city -- silent divergence from the first tick.
     * Until the loaded city is announced as an event upstream of the
     * recorder, the pair is refused rather than recorded wrongly.
     */
    void requireRecordableStart(
        const SaveCliOptions &options, bool recording);

} // namespace antwika::game
