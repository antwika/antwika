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

} // namespace antwika::game
