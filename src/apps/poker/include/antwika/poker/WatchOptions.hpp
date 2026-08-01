#pragma once

#include <chrono>
#include <span>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>

namespace antwika::poker
{

    /**
     * @brief How long one poker action takes when nobody said.
     *
     * One tick is one step of the poker loop -- a deal, or one player
     * being asked to act -- and at full speed a whole session is over
     * before a spectator has read a line of it.
     * A second per action is what makes it watchable, and
     * `--tick-delay-ms 0` is still there for the run that wants none.
     */
    inline constexpr std::chrono::milliseconds kDefaultTickDelay{1000};

    /**
     * @brief How the poker app was asked to pace what it draws.
     */
    struct WatchOptions
    {
        /**
         * @brief How long to hold each tick's frame.
         *
         * Defaults to kDefaultTickDelay, so an unadorned run is paced at
         * one poker action per second.
         * Pacing is wall-clock only: it is a sleep between ticks and
         * changes nothing the tick loop computes, so a session paced at
         * any speed reaches the same chip counts.
         */
        std::chrono::milliseconds tickDelay{kDefaultTickDelay};

        /**
         * @brief Whether to keep the last frame up until the window goes.
         *
         * Only somebody who named `--tick-delay-ms` with a positive value
         * is watching a window, and only they want the end held up.
         * It is deliberately not "the pacing is non-zero": the default
         * pacing applies to the terminal run too, and holding there would
         * hang under the headless backend, which never reports a close.
         */
        bool holdFinalFrame{false};

        bool operator==(const WatchOptions &other) const = default;
    };

    /**
     * @brief The flags this app accepts on top of the replay ones.
     *
     * Handed to antwika::app::runRecorded(), which parses them in the
     * same pass as its own.
     * Parsing `--tick-delay-ms` in a pass of its own is what stopped it
     * working: the other pass had already refused it.
     * @return The table, for a main() to pass on.
     */
    [[nodiscard]] std::span<const antwika::cli::FlagSpec> watchFlags();

    /**
     * @brief Read the pacing out of an already-parsed command line.
     *
     * Pacing is this app's concern rather than something every
     * replay-driven app needs, which is why the flag is this app's and
     * not antwika::replay's -- but it is parsed with the others.
     * @param parsed A command line parsed against a table that included
     * watchFlags().
     * @return The pacing asked for; a value that is not a non-negative
     * number is ignored, as it always was.
     */
    [[nodiscard]] WatchOptions watchOptionsFrom(
        const antwika::cli::CommandLine &parsed);

} // namespace antwika::poker
