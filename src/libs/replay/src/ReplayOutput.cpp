#include "ReplayOutput.hpp"

#include <antwika/replay/ReplayFormatError.hpp>

namespace antwika::replay::detail
{

    void writeReplayOrThrow(
        const ReplayWriter &writer,
        const std::vector<TickEvent> &events,
        std::ostream &out,
        const std::string &destination)
    {
        writer.write(events, out);

        // Flushed here rather than by the destructor, which cannot say.
        // A full disk fails on the flush, not on the open.
        out.flush();
        if (!out)
        {
            throw ReplayFormatError(
                "antwika::replay: could not write a replay: "
                + destination);
        }
    }

} // namespace antwika::replay::detail
