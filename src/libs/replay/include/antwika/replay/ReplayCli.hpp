#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/event/TickEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief The `--record`/`--replay` file paths a replay-driven app's
     * `main()` was invoked with.
     */
    struct ReplayCliOptions
    {
        /**
         * @brief Path to write the run's dispatched events to, if
         * `--record <path>` was given.
         */
        std::optional<std::string> recordPath;

        /**
         * @brief Path to load this run's input events from, if
         * `--replay <path>` was given.
         *
         * Unset means the caller should fall back to its own default,
         * e.g. an app's bundled demo replay.
         */
        std::optional<std::string> replayPath;
    };

    /**
     * @brief Parse the `--record <path>`/`--replay <path>` flags every
     * replay-driven app's `main(argc, argv)` accepts.
     * @param argc Argument count, as passed to `main()`.
     * @param argv Argument vector, as passed to `main()`.
     * @return The recognized options; flags missing their value, or not
     * recognized at all, are ignored.
     */
    [[nodiscard]] ReplayCliOptions parseReplayCliOptions(
        int argc, char **argv);

    /**
     * @brief Load a replay document from a file and decode its events.
     * @param path Path to the replay file to read.
     * @return The decoded events, in the order they were recorded.
     * @throws ReplayFormatError If the file can't be parsed as a replay
     * document.
     */
    [[nodiscard]] std::vector<TickEvent> loadReplayFile(
        const std::string &path);

    /**
     * @brief Write a run's events to a file as a replay document, with the
     * engine's own self-generated events filtered out first.
     *
     * `engine.tick` is always filtered: `Engine::step()` regenerates it
     * identically every run, live or replayed, so it was never really
     * input and must not be fed back in as replay input.
     * @param events The dispatched events to persist, in original order.
     * @param path Path to write the replay document to.
     * @param extraSelfGeneratedEventNames Additional event names to filter
     * out before writing, e.g. an app's own unconditional startup
     * announcement.
     */
    void saveReplayFile(
        std::vector<TickEvent> events,
        const std::string &path,
        std::span<const std::string_view> extraSelfGeneratedEventNames = {});

} // namespace antwika::replay
