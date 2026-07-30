#include "antwika/replay/ReplayCli.hpp"

#include <algorithm>
#include <fstream>

#include <antwika/engine/Events.hpp>

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
        const ReplayWriter writer(layout);
        writer.write(events, replayFile);
    }

} // namespace antwika::replay
