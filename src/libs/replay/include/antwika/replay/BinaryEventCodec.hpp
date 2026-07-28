#pragma once

#include "IEventCodec.hpp"

namespace antwika::replay
{

    // Encodes a TimedEvent as tick, then name, then payload.
    // The tick is 8 bytes, big-endian.
    // Name and payload are length-prefixed byte strings.
    // Each length prefix is a 4-byte big-endian integer.
    class BinaryEventCodec final : public IEventCodec
    {
    public:
        void encode(const TimedEvent &event, std::ostream &out) const override;
        [[nodiscard]] TimedEvent decode(std::istream &in) const override;
    };

} // namespace antwika::replay
