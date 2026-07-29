#include "antwika/replay/BinaryEventCodec.hpp"

#include "BinaryPrimitives.hpp"

namespace antwika::replay
{

    void BinaryEventCodec::encode(
        const TickEvent &event, std::ostream &out) const
    {
        detail::writeU64(event.tick, out);
        detail::writeString(event.event.name, out);
        detail::writeString(event.event.payload, out);
    }

    TickEvent BinaryEventCodec::decode(std::istream &in) const
    {
        TickEvent event;
        event.tick = detail::readU64(in);
        event.event.name = detail::readString(in);
        event.event.payload = detail::readString(in);
        return event;
    }

} // namespace antwika::replay
