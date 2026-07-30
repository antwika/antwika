#pragma once

#include <chrono>

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
     * @brief Parse the `--tick-delay-ms <n>` flag.
     *
     * Sits beside antwika::replay::parseReplayCliOptions rather than in
     * it: pacing is this app's concern, not something every
     * replay-driven app needs. Both ignore what they do not recognise,
     * so a command line can carry either.
     * @param argc Argument count, as passed to `main()`.
     * @param argv Argument vector, as passed to `main()`.
     * @return The recognized options; a flag missing its value, or
     * naming something that is not a number, is ignored.
     */
    [[nodiscard]] WatchOptions parseWatchOptions(int argc, char **argv);

} // namespace antwika::poker
