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

    std::span<const cli::FlagSpec> replayCliFlags()
    {
        return kReplayFlags;
    }

    ReplayCliOptions replayCliOptionsFrom(const cli::CommandLine &parsed)
    {
        ReplayCliOptions options;
        options.recordPath = parsed.value("--record");
        options.replayPath = parsed.value("--replay");
        options.helpRequested = parsed.has(cli::kHelpFlag);
        return options;
    } // GCOVR_EXCL_LINE

    std::vector<TickEvent> loadReplayFile(
        const std::string &path, CanvasCheck check)
    {
        std::ifstream replayFile =
            io::openToReadAs<ReplayFormatError>(path, "a replay");

        const ReplayReader reader(std::move(check));
        return reader.read(replayFile);
    }

    std::ofstream openReplayFile(const std::string &path)
    {
        return io::openToWriteAs<ReplayFormatError>(path, "a replay");
    }

    void saveReplayFile(
        const std::vector<TickEvent> &events,
        const std::string &path,
        std::optional<geometry::Size> canvas)
    {
        std::ofstream replayFile = openReplayFile(path);

        ReplayRecorder recorder(replayFile, path, canvas);
        for (const TickEvent &event : events)
        {
            recorder.handle(event);
        }
    }

}
