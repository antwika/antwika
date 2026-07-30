#include "antwika/replay/ReplayCli.hpp"

#include <algorithm>
#include <fstream>

#include <antwika/engine/Events.hpp>

#include "antwika/replay/ReplayFormatError.hpp"
#include "antwika/replay/ReplayReader.hpp"
#include "antwika/replay/ReplayWriter.hpp"

namespace antwika::replay
{

    ReplayCliOptions parseReplayCliOptions(int argc, char **argv)
    {
        ReplayCliOptions options;
        for (int i = 1; i < argc; ++i)
        {
            const std::string_view arg = argv[i];
            if (arg == "--record" && i + 1 < argc)
            {
                options.recordPath = argv[++i];
            }
            else if (arg == "--replay" && i + 1 < argc)
            {
                options.replayPath = argv[++i];
            }
        }
        return options;
    } // GCOVR_EXCL_LINE

    std::vector<TickEvent> loadReplayFile(const std::string &path)
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

        ReplayReader reader;
        return reader.read(replayFile);
    }

    void saveReplayFile(
        std::vector<TickEvent> events,
        const std::string &path,
        std::span<const std::string_view> extraSelfGeneratedEventNames,
        ReplayWriter::Layout layout)
    {
        std::erase_if(
            events,
            [extraSelfGeneratedEventNames](const TickEvent &event)
            {
                const auto &name = event.event.name;
                if (name == antwika::engine::events::kTick)
                {
                    return true;
                }
                return std::ranges::find(extraSelfGeneratedEventNames, name)
                       != extraSelfGeneratedEventNames.end();
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

        const ReplayWriter writer(layout);
        writer.write(events, replayFile);

        // Flushed here rather than by the destructor, which cannot say.
        // A full disk fails on the flush, not on the open.
        replayFile.flush();
        if (!replayFile)
        {
            throw ReplayFormatError(
                "antwika::replay: could not write a replay: " + path);
        }
    }

} // namespace antwika::replay
