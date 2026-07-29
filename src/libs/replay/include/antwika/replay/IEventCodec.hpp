#pragma once

#include <istream>
#include <ostream>

#include <antwika/event/TickEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief Converts TickEvent instances to and from a serialized form.
     */
    class IEventCodec
    {
    public:
        virtual ~IEventCodec() = default;

        /**
         * @brief Serialize a timed event to a stream.
         * @param event The event to encode.
         * @param out The stream to write the encoded bytes to.
         */
        virtual void encode(
            const TickEvent &event, std::ostream &out) const = 0;

        /**
         * @brief Deserialize a timed event from a stream.
         * @param in The stream to read the encoded bytes from.
         * @return The decoded event.
         */
        [[nodiscard]] virtual TickEvent decode(std::istream &in) const = 0;
    };

} // namespace antwika::replay
