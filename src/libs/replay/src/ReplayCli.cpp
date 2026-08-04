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
    } // namespace

    std::span<const cli::FlagSpec> replayCliFlags()
    {
        return kReplayFlags;
    }

    ReplayCliOptions replayCliOptionsFrom(const cli::CommandLine &parsed)
    {
        // Members assigned one at a time, not built as an aggregate.
        // An aggregate unwinds members it built if a later one throws.
        // gcov counts one such landing pad per member.
        // None are reachable without an allocation failure.
        // All share the one line with real branches.
        // An exclusion there would take those too.
        ReplayCliOptions options;
        options.recordPath = parsed.value("--record");
        options.replayPath = parsed.value("--replay");
        options.helpRequested = parsed.has(cli::kHelpFlag);
        return options;
    } // GCOVR_EXCL_LINE

    std::vector<TickEvent> loadReplayFile(
        const std::string &path, CanvasCheck check)
    {
        // A file that is not there is not a malformed document.
        // Unchecked, it reached the reader as an empty stream.
        // Which reported a valid replay as invalid JSON.
        std::ifstream replayFile =
            io::openToReadAs<ReplayFormatError>(path, "a replay");

        const ReplayReader reader(std::move(check));
        return reader.read(replayFile);
    }

    std::ofstream openReplayFile(const std::string &path)
    {
        // A path that will not take a header will not take a session.
        // Unchecked, a mistyped --record path used to lose one whole.
        return io::openToWriteAs<ReplayFormatError>(path, "a replay");
    }

    void saveReplayFile(
        const std::vector<TickEvent> &events,
        const std::string &path,
        std::optional<geometry::Size> canvas)
    {
        std::ofstream replayFile = openReplayFile(path);

        // Through the recorder rather than beside it.
        // So a whole recording and an appended one cannot drift apart.
        // The tick filter is written down once, over there.
        ReplayRecorder recorder(replayFile, path, canvas);
        for (const TickEvent &event : events)
        {
            recorder.handle(event);
        }
    }

} // namespace antwika::replay
