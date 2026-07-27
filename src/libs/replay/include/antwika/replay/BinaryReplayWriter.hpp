#pragma once

#include "IEventCodec.hpp"
#include "IReplayWriter.hpp"

namespace antwika::replay
{

    class BinaryReplayWriter final : public IReplayWriter
    {
    public:
        explicit BinaryReplayWriter(const IEventCodec &codec);

        void write(const std::vector<TimedEvent> &events, std::ostream &out) const override;

    private:
        const IEventCodec &codec;
    };

} // namespace antwika::replay
