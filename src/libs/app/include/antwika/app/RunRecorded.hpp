#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplayCli.hpp>

namespace antwika::app
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::replay::CommandLine;
    using antwika::replay::FlagSpec;
    using antwika::replay::ReplayCliOptions;

    /**
     * @brief What a recorded run hands to the body it runs.
     *
     * Everything here is owned by runRecorded() and outlives the body.
     */
    struct RecordedRun
    {
        /** @brief The `--record`/`--replay` paths the run was given. */
        const ReplayCliOptions &options;

        /**
         * @brief What the command line held, this app's own flags
         * included.
         *
         * Parsed once, against replayCliFlags() plus whatever extra
         * flags the caller passed.
         * A second parse of the same argv would refuse the first one's
         * flags, which is how `--tick-delay-ms` stopped working.
         */
        const CommandLine &commandLine;

        /**
         * @brief Where dispatched events go when nothing reads them.
         *
         * Every dispatched event has to go somewhere, and each app used
         * to keep a recorder there -- deep-copying both strings of every
         * event and holding them for the life of a process that ends
         * only when somebody closes a window. Nothing ever read them.
         */
        IEventSink &eventSink;

        /**
         * @brief The sink whose events are saved when the run ends.
         *
         * Set only for a `--record` run, so a run with no end does not
         * accumulate every event it ever dispatched. Pass it straight
         * into a bootstrap()'s replayRecorder.
         */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder;
    };

    /**
     * @brief Parse `--record`/`--replay`, run a session, and save the
     * recording.
     *
     * This is the shape every replay-driven main() had a copy of. It
     * exists so that a main() can be a single call with no branch in it:
     * the argument parsing, the sink for unread events, the recorder,
     * the catch and the save epilogue are all things worth testing, and
     * an application's main.cpp is deliberately excluded from the
     * coverage report.
     *
     * Catching is runGuarded()'s job, and this calls it twice: once
     * around the parse and the body, once around the save. Two guards
     * rather than one is what lets a failed `--record` run still save
     * what it got to, and lets an unwritable path be reported rather
     * than thrown out of a main() that has no catch of its own.
     *
     * `--help` is answered here rather than run: the table this parsed
     * against is rendered to `help` and the body is never called, which
     * is what makes every app's `--help` work without a branch in its
     * `main()`. A `--record` given alongside it writes nothing, since
     * asking what the flags are is not a session.
     *
     * @param argc Argument count, as passed to main().
     * @param argv Argument vector, as passed to main().
     * @param name The program's name, used to prefix a failure and to
     * head the help text.
     * @param body The session to run.
     * @param extraFlags This app's own flags, parsed in the same pass as
     * the replay ones so that neither can refuse the other.
     * @param errors Where a failure is reported.
     * @param help Where `--help` is answered; standard output, because a
     * question that was asked for is not a diagnostic.
     * @return EXIT_SUCCESS, or EXIT_FAILURE if the body threw.
     */
    int runRecorded(
        int argc,
        char **argv,
        std::string_view name,
        const std::function<void(const RecordedRun &)> &body,
        std::span<const FlagSpec> extraFlags = {},
        std::ostream &errors = std::cerr,
        std::ostream &help = std::cout);

    /**
     * @brief Load the events a run is seeded with.
     *
     * An app either has a demo replay to fall back on or starts from
     * nothing, and saying which used to be a branch in a main().
     *
     * @param replayPath What `--replay` named, if anything.
     * @param fallback The app's own default; empty means start empty.
     * @return The events to seed a ReplaySource with.
     * @throws antwika::replay::ReplayFormatError If a named file cannot
     * be read or parsed.
     */
    [[nodiscard]] std::vector<TickEvent> scriptedEvents(
        const std::optional<std::string> &replayPath,
        std::string_view fallback = {});

} // namespace antwika::app
