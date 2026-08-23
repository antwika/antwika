#include "antwika/app/RunRecorded.hpp"

#include <fstream>

#include <antwika/event/Event.hpp>
#include <antwika/replay/ReplayRecorder.hpp>

#include "antwika/app/RunCatchingErrors.hpp"

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::replay::ReplayRecorder;

    namespace
    {
        class DiscardedEvents final : public IEventSink
        {
        public:
            void handle(const Event &) override
            {
            }
        };
    }

    int runRecorded(
        int argc,
        char **argv,
        std::string_view name,
        const std::function<void(const RunContext &)> &body,
        std::span<const FlagSpec> extraFlags,
        std::ostream &errors,
        std::ostream &help)
    {
        DiscardedEvents eventSink;

        return runCatchingErrors(
            name,
            [&]
            {
                std::vector<FlagSpec> table(
                    antwika::replay::getReplayCliFlags().begin(),
                    antwika::replay::getReplayCliFlags().end());
                table.insert(
                    table.end(), extraFlags.begin(), extraFlags.end());

                const CommandLine parsedLine =
                    antwika::cli::parseCommandLine(argc, argv, table);
                const auto options =
                    antwika::replay::replayCliOptionsFrom(parsedLine);

                if (options.helpRequested)
                {
                    help << antwika::cli::getHelpText(name, table);
                    return;
                }

                std::optional<std::ofstream> recordFile;
                std::optional<ReplayRecorder> recorder;
                if (options.recordPath)
                {
                    recordFile = antwika::replay::getOpenReplayFile(
                        *options.recordPath);
                    recorder.emplace(*recordFile, *options.recordPath);
                }

                RunContext run{
                    .options = options,
                    .commandLine = parsedLine,
                    .eventSink = eventSink,
                    .replayRecorderSink = std::nullopt};
                if (recorder)
                {
                    run.replayRecorderSink = *recorder;
                }

                body(run);
            },
            errors);
    }

    std::vector<TickEvent> getLoadReplayEvents(
        const std::optional<std::string> &replayPath,
        std::string_view fallback)
    {
        if (replayPath)
        {
            return antwika::replay::getLoadReplayFile(*replayPath);
        }

        if (fallback.empty())
        {
            return {};
        }

        return antwika::replay::getLoadReplayFile(std::string(fallback));
    }

}
