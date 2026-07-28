#pragma once

#include <ostream>
#include <vector>

#include "IEventCodec.hpp"

namespace antwika::replay
{

    /**
     * @brief Writes a sequence of TimedEvent instances to a binary replay
     * stream, readable by BinaryReplayReader.
     */
    class BinaryReplayWriter final
    {
    public:
        /**
         * @brief Construct the writer over a codec used to encode events.
         * @param codec Codec used to encode each event in the stream.
         */
        explicit BinaryReplayWriter(const IEventCodec &codec);

        /**
         * @brief Write the replay header and every event to a stream.
         * @param events The events to write, in the order they occurred.
         * @param out The stream to write the encoded bytes to.
         */
        void write(const std::vector<TimedEvent> &events, std::ostream &out) const;

    private:
        const IEventCodec &codec;
    };

} // namespace antwika::replay
