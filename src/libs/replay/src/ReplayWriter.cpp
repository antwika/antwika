#include "antwika/replay/ReplayWriter.hpp"

#include <nlohmann/json.hpp>

#include <antwika/replay/EventJson.hpp>
#include <antwika/replay/ReplayHeader.hpp>
#include <antwika/replay/ReplayJson.hpp>

namespace antwika::replay
{

    ReplayWriter::ReplayWriter(
        std::optional<geometry::Size> canvas) noexcept
        : canvas(canvas)
    {
    }

    void ReplayWriter::writeHeader(std::ostream &out) const
    {
        out << replayHeaderToJson(ReplayHeader{.canvas = canvas}).dump()
            << '\n';
    }

    void ReplayWriter::writeRecord(
        const TickEvent &event, std::ostream &out) const
    {
        out << nlohmann::json(event).dump() << '\n';
    }

    void ReplayWriter::write(
        const std::vector<TickEvent> &events, std::ostream &out) const
    {
        writeHeader(out);
        for (const TickEvent &event : events)
        {
            writeRecord(event, out);
        }
    }

} // namespace antwika::replay
