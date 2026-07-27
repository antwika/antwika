#pragma once

#include "IEventCodec.hpp"

namespace antwika::replay
{

    // Encodes a TimedEvent as: tick (8 bytes, big-endian), then name and
    // payload as length-prefixed (4-byte big-endian length) byte strings.
    class BinaryEventCodec final : public IEventCodec
    {
    public:
        void encode(const TimedEvent &event, std::ostream &out) const override;
        [[nodiscard]] TimedEvent decode(std::istream &in) const override;
    };

} // namespace antwika::replay
