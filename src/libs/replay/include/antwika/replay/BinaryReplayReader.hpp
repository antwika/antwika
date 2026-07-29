#pragma once

#include <istream>
#include <vector>

#include "IEventCodec.hpp"

namespace antwika::replay
{

    /**
     * @brief Reads the format BinaryReplayWriter produces.
     *
     * Throws ReplayFormatError on a malformed stream (see
     * ReplayFormatError.hpp): a stream missing the expected magic bytes, an
     * unsupported format version, or a stream that ends before the data it
     * claims to hold.
     */
    class BinaryReplayReader final
    {
    public:
        /**
         * @brief Construct the reader over a codec used to decode events.
         * @param codec Codec used to decode each event in the stream.
         */
        explicit BinaryReplayReader(const IEventCodec &codec);

        /**
         * @brief Read and decode every event from a binary replay stream.
         * @param in The stream to read from.
         * @return The decoded events, in the order they were recorded.
         * @throws ReplayFormatError If the stream is malformed.
         */
        [[nodiscard]] std::vector<TickEvent> read(std::istream &in) const;

    private:
        const IEventCodec &codec;
    };

} // namespace antwika::replay
