#include "antwika/replay/ReplayRecorder.hpp"

#include <utility>

#include <antwika/engine/Events.hpp>
#include <antwika/io/File.hpp>
#include <antwika/replay/ReplayFormatError.hpp>

namespace antwika::replay
{

    namespace
    {
        void requireStreamTookIt(
            std::ostream &out, const std::string &destination)
        {
            // Flushed per line rather than once at the end on purpose.
            // A recording nobody flushed is one a kill loses.
            // The flush-then-check itself is antwika::io's, stated once.
            io::requireStreamTookAs<ReplayFormatError>(
                out, "a replay", destination);
        }
    } // namespace

    ReplayRecorder::ReplayRecorder(
        std::ostream &out,
        std::string destination,
        std::optional<geometry::Size> canvas)
        : out(out), destination(std::move(destination)), writer(canvas)
    {
        writer.writeHeader(out);
        requireStreamTookIt(out, this->destination);
    }

    void ReplayRecorder::handle(const event::TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            return;
        }

        writer.writeRecord(event, out);
        requireStreamTookIt(out, destination);
    }

} // namespace antwika::replay
