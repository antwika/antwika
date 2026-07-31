#include "antwika/app/RunRecorded.hpp"

#include <cstdlib>
#include <exception>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEventRecorder.hpp>

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::event::TickEventRecorder;

    namespace
    {
        /**
         * @brief Sink for the events nothing in an app reads.
         */
        class DiscardedEvents final : public IEventSink
        {
        public:
            void handle(const Event &) override
            {
            }
        };
    } // namespace

    int runRecorded(
        int argc,
        char **argv,
        std::string_view name,
        const std::function<void(const RecordedRun &)> &body,
        std::span<const FlagSpec> extraFlags,
        std::ostream &errors,
        std::ostream &help)
    {
        DiscardedEvents eventSink;
        TickEventRecorder replayRecorder;

        // Held out here because the epilogue below still needs it.
        // Parsing itself is inside the try, deliberately.
        std::optional<std::string> recordPath;

        int exitCode = EXIT_SUCCESS;
        const auto report = [&errors, &name, &exitCode](
                                const std::exception &error)
        {
            errors << name << ": " << error.what() << '\n';
            exitCode = EXIT_FAILURE;
        };

        try
        {
            // A refused flag is a failed run, not a crash.
            // Parsed outside the try it reaches std::terminate.
            // That unwinds nothing and names no program.
            //
            // One parse, against one table.
            // A second pass refuses whatever the first pass accepted.
            std::vector<FlagSpec> table(
                antwika::replay::replayCliFlags().begin(),
                antwika::replay::replayCliFlags().end());
            table.insert(table.end(), extraFlags.begin(), extraFlags.end());

            const CommandLine parsed =
                antwika::replay::parseCommandLine(argc, argv, table);
            const auto options =
                antwika::replay::replayCliOptionsFrom(parsed);

            // --help is a question, not a run.
            // Answering it starts no session and writes no recording.
            // recordPath is left unset, so the epilogue below skips.
            if (options.helpRequested)
            {
                help << antwika::replay::helpText(name, table);
            }
            else
            {
                recordPath = options.recordPath;

                RecordedRun run{
                    .options = options,
                    .commandLine = parsed,
                    .eventSink = eventSink,
                    .replayRecorder = std::nullopt};
                if (options.recordPath)
                {
                    run.replayRecorder = replayRecorder;
                }

                body(run);
            }
        }
        catch (const std::exception &error)
        {
            report(error);
        }

        // After the catch, so a run that failed still saves what it got.
        // A run refused at the command line has nothing to save.
        //
        // Saving throws on its own account too.
        // An unwritable path, or a full disk.
        // Uncaught, that throw leaves runRecorded() entirely.
        // A main() has no catch of its own, by design.
        // So the process terminated rather than saying which path.
        if (recordPath)
        {
            try
            {
                antwika::replay::saveReplayFile(
                    replayRecorder.getEvents(), *recordPath);
            }
            // gcov -b tags this handler's non-matching edge.
            // It is taken only by a throw that is not a std::exception.
            // The catch above is reached by one, from the caller's body.
            // This try calls no caller code at all.
            // saveReplayFile throws ReplayFormatError, and nothing else.
            // See docs/confirming-unreachable-branches.md.
            catch (const std::exception &error) // GCOVR_EXCL_LINE
            {
                report(error);
            }
        }

        return exitCode;
    }

    std::vector<TickEvent> scriptedEvents(
        const std::optional<std::string> &replayPath,
        std::string_view fallback)
    {
        if (replayPath)
        {
            return antwika::replay::loadReplayFile(*replayPath);
        }

        if (fallback.empty())
        {
            return {};
        }

        return antwika::replay::loadReplayFile(std::string(fallback));
    }

} // namespace antwika::app
