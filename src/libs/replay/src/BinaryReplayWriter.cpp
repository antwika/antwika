#include "antwika/replay/BinaryReplayWriter.hpp"

#include "BinaryPrimitives.hpp"
#include "ReplayFormat.hpp"

namespace antwika::replay
{

    BinaryReplayWriter::BinaryReplayWriter(const IEventCodec &codec)
        : codec(codec)
    {
    }

    void BinaryReplayWriter::write(
        const std::vector<TickEvent> &events, std::ostream &out) const
    {
        out.write(
            detail::kReplayMagic.data(),
            static_cast<std::streamsize>(detail::kReplayMagic.size()));
        detail::writeU32(detail::kReplayFormatVersion, out);
        detail::writeU32(static_cast<std::uint32_t>(events.size()), out);

        for (const auto &event : events)
        {
            codec.encode(event, out);
        }
    }

} // namespace antwika::replay
