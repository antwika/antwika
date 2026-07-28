#pragma once

#include <istream>
#include <vector>

#include "IEventCodec.hpp"

namespace antwika::replay
{

    // Reads the format BinaryReplayWriter produces.
    // Throws ReplayFormatError on a malformed stream (see ReplayFormatError.hpp).
    // That includes a stream missing the expected magic bytes.
    // It also includes an unsupported format version.
    // It also includes a stream that ends before the data it claims to hold.
    class BinaryReplayReader final
    {
    public:
        explicit BinaryReplayReader(const IEventCodec &codec);

        [[nodiscard]] std::vector<TimedEvent> read(std::istream &in) const;

    private:
        const IEventCodec &codec;
    };

} // namespace antwika::replay
