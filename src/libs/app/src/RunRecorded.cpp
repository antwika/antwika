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
        std::ostream &errors)
    {
        DiscardedEvents eventSink;
        TickEventRecorder replayRecorder;

        // Held out here because the epilogue below still needs it.
        // Parsing itself is inside the try, deliberately.
        std::optional<std::string> recordPath;

        int exitCode = EXIT_SUCCESS;
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
        catch (const std::exception &error)
        {
            errors << name << ": " << error.what() << '\n';
            exitCode = EXIT_FAILURE;
        }

        // After the catch, so a run that failed still saves what it got.
        // A run refused at the command line has nothing to save.
        if (recordPath)
        {
            antwika::replay::saveReplayFile(
                replayRecorder.getEvents(), *recordPath);
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
