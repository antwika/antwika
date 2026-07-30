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
        std::ostream &errors)
    {
        const auto options = antwika::replay::parseReplayCliOptions(
            argc, argv);

        DiscardedEvents eventSink;
        TickEventRecorder replayRecorder;

        RecordedRun run{
            .options = options,
            .eventSink = eventSink,
            .replayRecorder = std::nullopt};
        if (options.recordPath)
        {
            run.replayRecorder = replayRecorder;
        }

        int exitCode = EXIT_SUCCESS;
        try
        {
            body(run);
        }
        catch (const std::exception &error)
        {
            errors << name << ": " << error.what() << '\n';
            exitCode = EXIT_FAILURE;
        }

        // After the catch, so a run that failed still saves what it got.
        if (options.recordPath)
        {
            antwika::replay::saveReplayFile(
                replayRecorder.getEvents(), *options.recordPath);
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
