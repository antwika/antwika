#pragma once

#include <istream>
#include <ostream>

#include <antwika/event/TimedEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TimedEvent;

    /**
     * @brief Converts TimedEvent instances to and from a serialized form.
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
            const TimedEvent &event, std::ostream &out) const = 0;

        /**
         * @brief Deserialize a timed event from a stream.
         * @param in The stream to read the encoded bytes from.
         * @return The decoded event.
         */
        [[nodiscard]] virtual TimedEvent decode(std::istream &in) const = 0;
    };

} // namespace antwika::replay
