#include "antwika/replay/ReplayCli.hpp"

#include <array>
#include <fstream>
#include <utility>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/io/File.hpp>

#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplayRecorder.hpp"

namespace antwika::replay
{

    namespace
    {
        constexpr std::array kReplayFlags{
            cli::FlagSpec{
                .name = "--record",
                .valueName = "<path>",
                .help = "Write this run's input events to <path>.",
            },
            cli::FlagSpec{
                .name = "--replay",
                .valueName = "<path>",
                .help = "Load this run's input events from <path>.",
            },
        };
    }

    std::span<const cli::FlagSpec> getReplayCliFlags()
    {
        return kReplayFlags;
    }

    ReplayCliOptions replayCliOptionsFrom(const cli::CommandLine &parsedLine)
    {
        ReplayCliOptions options;
        options.recordPath = parsedLine.getValue("--record");
        options.replayPath = parsedLine.getValue("--replay");
        options.helpRequested = parsedLine.has(cli::kHelpFlag);
        return options;
    } // GCOVR_EXCL_LINE

    std::vector<TickEvent> getLoadReplayFile(
        const std::string &path, CanvasCheckOptions check)
    {
        std::ifstream replayFile =
            io::openToReadAs<ReplayFormatError>(path, "a replay");

        const ReplayReader reader(std::move(check));
        return reader.read(replayFile);
    }

    std::ofstream getOpenReplayFile(const std::string &path)
    {
        return io::openToWriteAs<ReplayFormatError>(path, "a replay");
    }

    void saveReplayFile(
        const std::vector<TickEvent> &events,
        const std::string &path,
        std::optional<geometry::Size> canvasSize)
    {
        std::ofstream replayFile = getOpenReplayFile(path);

        ReplayRecorder recorder(replayFile, path, canvasSize);
        for (const TickEvent &event : events)
        {
            recorder.handle(event);
        }
    }

}
