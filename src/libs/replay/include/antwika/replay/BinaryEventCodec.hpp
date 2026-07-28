#pragma once

#include "IEventCodec.hpp"

namespace antwika::replay
{

    /**
     * @brief IEventCodec that encodes a TimedEvent as tick, then name, then
     * payload.
     *
     * The tick is 8 bytes, big-endian. Name and payload are length-prefixed
     * byte strings, each length prefix being a 4-byte big-endian integer.
     */
    class BinaryEventCodec final : public IEventCodec
    {
    public:
        /**
         * @brief Serialize a timed event in the binary format described above.
         * @param event The event to encode.
         * @param out The stream to write the encoded bytes to.
         */
        void encode(const TimedEvent &event, std::ostream &out) const override;

        /**
         * @brief Deserialize a timed event from the binary format above.
         * @param in The stream to read the encoded bytes from.
         * @return The decoded event.
         */
        [[nodiscard]] TimedEvent decode(std::istream &in) const override;
    };

} // namespace antwika::replay
