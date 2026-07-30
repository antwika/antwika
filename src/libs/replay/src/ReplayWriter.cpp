#include "antwika/replay/ReplayWriter.hpp"

#include <antwika/replay/ReplayJson.hpp>

namespace antwika::replay
{

    namespace
    {
        // What nlohmann's dump() takes for a width.
        // A negative one is no indentation, and no newlines with it.
        constexpr int kNoIndent = -1;
        constexpr int kTwoSpaces = 2;
    } // namespace

    ReplayWriter::ReplayWriter(ReplayWriter::Layout layout) noexcept
        : layout(layout)
    {
    }

    void ReplayWriter::write(
        const std::vector<TickEvent> &events, std::ostream &out) const
    {
        const auto indent =
            layout == Layout::Pretty ? kTwoSpaces : kNoIndent;

        out << replayToJson(events).dump(indent) << '\n';
    }

} // namespace antwika::replay
