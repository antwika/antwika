#include "antwika/replay/ReplayWriter.hpp"

#include <nlohmann/json.hpp>

#include <antwika/replay/EventJson.hpp>
#include <antwika/replay/ReplayHeader.hpp>
#include <antwika/replay/ReplayJson.hpp>

namespace antwika::replay
{

    ReplayWriter::ReplayWriter(
        std::optional<geometry::Size> canvasSize) noexcept
        : canvas(canvasSize)
    {
    }

    void ReplayWriter::writeHeader(std::ostream &outputStream) const
    {
        outputStream
            << getReplayHeaderToJson(ReplayHeader{.canvasSize = canvas}).dump()
            << '\n';
    }

    void ReplayWriter::writeRecord(
        const TickEvent &event, std::ostream &outputStream) const
    {
        outputStream << nlohmann::json(event).dump() << '\n';
    }

    void ReplayWriter::write(
        const std::vector<TickEvent> &events, std::ostream &outputStream) const
    {
        writeHeader(outputStream);
        for (const TickEvent &event : events)
        {
            writeRecord(event, outputStream);
        }
    }

}
