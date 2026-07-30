#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/replay/CanvasCheck.hpp>
#include <antwika/replay/ReplayWriter.hpp>

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
     * @param check What to compare the document's recorded canvas with,
     * and where to warn when the two differ; by default neither, which
     * loads the file without looking at its canvas.
     * @return The decoded events, in the order they were recorded.
     * @throws ReplayFormatError If the file can't be opened at all, or
     * can't be parsed as a replay document. The two say so differently: a
     * file that is not there is not a malformed one.
     * A canvas that differs is neither, and only warns.
     */
    [[nodiscard]] std::vector<TickEvent> loadReplayFile(
        const std::string &path, CanvasCheck check = {});

    /**
     * @brief Write a run's events to a file as a replay document, with the
     * engine's own self-generated events filtered out first.
     *
     * `engine.tick` is always filtered: `Engine::step()` regenerates it
     * identically every run, live or replayed, so it was never really
     * input and must not be fed back in as replay input.
     *
     * It is also the only name filtered here.
     * Apps used to pass the names of their own startup announcements too;
     * those are log lines now, so the tick is all that is regenerated.
     * @param events The dispatched events to persist, in original order.
     * @param path Path to write the replay document to.
     * @param canvas The canvas this run laid its input out against,
     * recorded in the document so a later run playing it back against a
     * different one can be told. Unset writes no canvas, which is what a
     * recording of an app with no pointer input has to say.
     * @param layout How much whitespace to write; compact by default,
     * since a recorded session is written to be replayed rather than
     * read, and an interactive one gets long enough for the indentation
     * to be most of the file. Pass Pretty for a replay meant to be
     * checked in and edited by hand.
     * @throws ReplayFormatError If the file can't be opened, or if the
     * bytes can't be written once it is. A recording is written once, at
     * the end of a run, so failing quietly here loses the whole session.
     */
    void saveReplayFile(
        std::vector<TickEvent> events,
        const std::string &path,
        std::optional<gfx::Size> canvas = std::nullopt,
        ReplayWriter::Layout layout = ReplayWriter::Layout::Compact);

} // namespace antwika::replay
