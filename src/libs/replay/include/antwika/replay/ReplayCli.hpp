#pragma once

#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>
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

        /**
         * @brief Whether `--help` was asked for.
         *
         * A program that sees this should print helpText() and stop,
         * rather than run whatever else it was told.
         */
        bool helpRequested = false;
    };

    /**
     * @brief The flags every replay-driven app accepts.
     * @return The table, for an app to parse against directly or to
     * extend with flags of its own.
     *
     * An app with extra flags concatenates this with its own table,
     * calls antwika::cli::parseCommandLine() once, and hands the result
     * to replayCliOptionsFrom() -- rather than parsing twice, which
     * would make one of the two passes refuse the other's flags.
     */
    [[nodiscard]] std::span<const cli::FlagSpec> replayCliFlags();

    /**
     * @brief Pick the replay options out of an already-parsed command
     * line.
     * @param parsed A command line parsed against a table that included
     * replayCliFlags().
     * @return The replay options it holds.
     */
    [[nodiscard]] ReplayCliOptions replayCliOptionsFrom(
        const cli::CommandLine &parsed);

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
     * @brief Open a file to append a recording to, as a `--record` run
     * does before its first tick.
     * @param path Path to write the recording to.
     * @return The opened stream, for a ReplayRecorder to write into.
     * @throws ReplayFormatError If the file can't be opened.
     *
     * Opened before the session rather than after it: a recording is
     * appended as the run goes, so a path that will not take one is
     * something to say at the start rather than at the end of an hour.
     */
    [[nodiscard]] std::ofstream openReplayFile(const std::string &path);

    /**
     * @brief Write a run's events to a file as a replay, with the
     * engine's own self-generated events filtered out first.
     *
     * The whole-recording-at-once counterpart of ReplayRecorder, which
     * it is written through -- so the two agree on what a recording
     * holds by construction.
     * A `--record` run uses the recorder; this is for a caller that
     * already holds every event, which is what a test usually does.
     *
     * `engine.tick` is always filtered: `Engine::step()` regenerates it
     * identically every run, live or replayed, so it was never really
     * input and must not be fed back in as replay input.
     *
     * It is also the only name filtered here.
     * Apps used to pass the names of their own startup announcements too;
     * those are log lines now, so the tick is all that is regenerated.
     * @param events The dispatched events to persist, in original order.
     * @param path Path to write the replay to.
     * @param canvas The canvas this run laid its input out against,
     * recorded in the header so a later run playing it back against a
     * different one can be told. Unset writes no canvas, which is what a
     * recording of an app with no pointer input has to say.
     * @throws ReplayFormatError If the file can't be opened, or if the
     * bytes can't be written once it is.
     */
    void saveReplayFile(
        const std::vector<TickEvent> &events,
        const std::string &path,
        std::optional<gfx::Size> canvas = std::nullopt);

} // namespace antwika::replay
