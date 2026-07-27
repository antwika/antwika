#pragma once

#include <ostream>
#include <vector>

#include "IEventCodec.hpp"

namespace antwika::replay
{

    class BinaryReplayWriter final
    {
    public:
        explicit BinaryReplayWriter(const IEventCodec &codec);

        void write(const std::vector<TimedEvent> &events, std::ostream &out) const;

    private:
        const IEventCodec &codec;
    };

} // namespace antwika::replay
