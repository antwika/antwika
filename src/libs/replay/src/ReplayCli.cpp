#include "antwika/replay/ReplayCli.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <utility>

#include <antwika/engine/Events.hpp>

#include "antwika/replay/CommandLine.hpp"
#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplayWriter.hpp"
#include "ReplayOutput.hpp"

namespace antwika::replay
{

    namespace
    {
        constexpr std::array kReplayFlags{
            FlagSpec{
                .name = "--record",
                .valueName = "<path>",
                .help = "Write this run's input events to <path>.",
            },
            FlagSpec{
                .name = "--replay",
                .valueName = "<path>",
                .help = "Load this run's input events from <path>.",
            },
        };
    } // namespace

    std::span<const FlagSpec> replayCliFlags()
    {
        return kReplayFlags;
    }

    ReplayCliOptions replayCliOptionsFrom(const CommandLine &parsed)
    {
        return ReplayCliOptions{
            .recordPath = parsed.value("--record"),
            .replayPath = parsed.value("--replay"),
            .helpRequested = parsed.has(kHelpFlag),
        };
    } // GCOVR_EXCL_LINE

    ReplayCliOptions parseReplayCliOptions(int argc, char **argv)
    {
        return replayCliOptionsFrom(
            parseCommandLine(argc, argv, replayCliFlags()));
    }

    std::vector<TickEvent> loadReplayFile(
        const std::string &path, CanvasCheck check)
    {
        std::ifstream replayFile(path);

        // A file that is not there is not a malformed document.
        // Unchecked, it reached the reader as an empty stream.
        // Which reported a valid replay as invalid JSON.
        if (!replayFile.is_open())
        {
            throw ReplayFormatError(
                "antwika::replay: could not open a replay to read: "
                + path);
        }

        const ReplayReader reader(std::move(check));
        return reader.read(replayFile);
    }

    void saveReplayFile(
        std::vector<TickEvent> events,
        const std::string &path,
        std::optional<gfx::Size> canvas,
        ReplayWriter::Layout layout)
    {
        std::erase_if(
            events,
            [](const TickEvent &event)
            {
                return event.event.name == antwika::engine::events::kTick;
            });

        std::ofstream replayFile(path);

        // A --record run only writes once the run has ended.
        // So an unwritable path used to lose a whole session in silence.
        if (!replayFile.is_open())
        {
            throw ReplayFormatError(
                "antwika::replay: could not open a replay to write: "
                + path);
        }

        const ReplayWriter writer(layout, canvas);
        detail::writeReplayOrThrow(writer, events, replayFile, path);
    }

} // namespace antwika::replay
