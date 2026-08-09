#include "antwika/app/RunRecorded.hpp"

#include <fstream>

#include <antwika/event/Event.hpp>
#include <antwika/replay/ReplayRecorder.hpp>

#include "antwika/app/RunGuarded.hpp"

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
        const std::function<void(const RecordedRun &)> &body,
        std::span<const FlagSpec> extraFlags,
        std::ostream &errors,
        std::ostream &help)
    {
        DiscardedEvents eventSink;

        return runGuarded(
            name,
            [&]
            {
                std::vector<FlagSpec> table(
                    antwika::replay::replayCliFlags().begin(),
                    antwika::replay::replayCliFlags().end());
                table.insert(
                    table.end(), extraFlags.begin(), extraFlags.end());

                const CommandLine parsed =
                    antwika::cli::parseCommandLine(argc, argv, table);
                const auto options =
                    antwika::replay::replayCliOptionsFrom(parsed);

                if (options.helpRequested)
                {
                    help << antwika::cli::helpText(name, table);
                    return;
                }

                std::optional<std::ofstream> recordFile;
                std::optional<ReplayRecorder> recorder;
                if (options.recordPath)
                {
                    recordFile = antwika::replay::openReplayFile(
                        *options.recordPath);
                    recorder.emplace(*recordFile, *options.recordPath);
                }

                RecordedRun run{
                    .options = options,
                    .commandLine = parsed,
                    .eventSink = eventSink,
                    .replayRecorder = std::nullopt};
                if (recorder)
                {
                    run.replayRecorder = *recorder;
                }

                body(run);
            },
            errors);
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

}
