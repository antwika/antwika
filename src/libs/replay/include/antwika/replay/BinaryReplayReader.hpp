#pragma once

#include "IEventCodec.hpp"
#include "IReplayReader.hpp"

namespace antwika::replay
{

    // Reads the format BinaryReplayWriter produces. Throws ReplayFormatError
    // (see ReplayFormatError.hpp) if the stream doesn't start with the
    // expected magic bytes, declares an unsupported format version, or ends
    // before the data it claims to contain.
    class BinaryReplayReader final : public IReplayReader
    {
    public:
        explicit BinaryReplayReader(const IEventCodec &codec);

        [[nodiscard]] std::vector<TimedEvent> read(std::istream &in) const override;

    private:
        const IEventCodec &codec;
    };

} // namespace antwika::replay
