#pragma once

#include <chrono>
#include <span>

#include <antwika/replay/CommandLine.hpp>
#include <antwika/replay/FlagSpec.hpp>

namespace antwika::poker
{

    /**
     * @brief How the poker app was asked to pace what it draws.
     */
    struct WatchOptions
    {
        /**
         * @brief How long to hold each tick's frame.
         *
         * Zero, the default, means nobody asked to watch: the session
         * runs at full speed and the window is not held open once it
         * ends. That is what keeps `antwika_poker` a terminal program
         * under the headless backend, which never reports a close.
         */
        std::chrono::milliseconds tickDelay{0};

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
    [[nodiscard]] std::span<const antwika::replay::FlagSpec> watchFlags();

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
        const antwika::replay::CommandLine &parsed);

} // namespace antwika::poker
