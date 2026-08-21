#include "antwika/replay/ReplayRecorder.hpp"

#include <utility>

#include <antwika/engine/Events.hpp>
#include <antwika/io/File.hpp>
#include <antwika/replay/ReplayFormatError.hpp>

namespace antwika::replay
{

    namespace
    {
        void requireStreamOk(
            std::ostream &outputStream, const std::string &destination)
        {
            io::requireStreamOkAs<ReplayFormatError>(
                outputStream, "a replay", destination);
        }
    }

    ReplayRecorder::ReplayRecorder(
        std::ostream &outputStream,
        std::string destination,
        std::optional<geometry::Size> canvasSize)
        : outputStream(outputStream),
          destination(std::move(destination)),
          writer(canvasSize)
    {
        writer.writeHeader(outputStream);
        requireStreamOk(outputStream, this->destination);
    }

    void ReplayRecorder::handle(const event::TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            return;
        }

        writer.writeRecord(event, outputStream);
        requireStreamOk(outputStream, destination);
    }

}
