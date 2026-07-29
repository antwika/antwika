#include "antwika/replay/ReplayWriter.hpp"

#include <antwika/replay/ReplayJson.hpp>

namespace antwika::replay
{

    void ReplayWriter::write(
        const std::vector<TickEvent> &events, std::ostream &out) const
    {
        out << replayToJson(events).dump(2) << '\n';
    }

} // namespace antwika::replay
