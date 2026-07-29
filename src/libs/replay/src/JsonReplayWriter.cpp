#include "antwika/replay/JsonReplayWriter.hpp"

#include <antwika/replay/ReplayJson.hpp>

namespace antwika::replay
{

    void JsonReplayWriter::write(
        const std::vector<TimedEvent> &events, std::ostream &out) const
    {
        out << replayToJson(events).dump(2) << '\n';
    }

} // namespace antwika::replay
