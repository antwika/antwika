#include "antwika/replay/ReplayRecorder.hpp"

#include <utility>

#include <antwika/engine/Events.hpp>
#include <antwika/replay/ReplayFormatError.hpp>

namespace antwika::replay
{

    namespace
    {
        void requireStreamTookIt(
            std::ostream &out, const std::string &destination)
        {
            // Flushed here rather than by the destructor.
            // A destructor cannot say that it failed.
            // A full disk fails on the flush, not on the open.
            // And a recording nobody flushed is one a kill loses.
            out.flush();
            if (!out)
            {
                throw ReplayFormatError(
                    "antwika::replay: could not write a replay: "
                    + destination);
            }
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
