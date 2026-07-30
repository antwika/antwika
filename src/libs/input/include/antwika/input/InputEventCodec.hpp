#pragma once

#include <optional>

#include <antwika/event/Event.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    /**
     * @brief The codec, encoding each edge as JSON under an input.* name.
     *
     * Stateless, and therefore freely copyable: what an event encodes to
     * depends on the event alone, which is what makes a replay's contents
     * a function of its input rather than of when it was recorded.
     */
    class InputEventCodec final : public IInputEventCodec
    {
    public:
        /**
         * @brief Encode an input event as a named, JSON-payloaded event.
         * @param event The edge to encode.
         * @return The event to dispatch, and to persist as-is.
         * @throws InputError If the edge names a key or a button outside
         * its enumeration, which decoding the result would reject too.
         */
        [[nodiscard]] Event encode(const InputEvent &event) const override;

        /**
         * @brief Decode an event back into the edge it was encoded from.
         * @param event The event to decode.
         * @return The edge it encodes, or nullopt when the event's name is
         * not one of antwika::input::events.
         * @throws InputError If the name is one of this library's but the
         * payload is not valid JSON of the shape that name requires, or
         * names a key or button that does not exist.
         */
        [[nodiscard]] std::optional<InputEvent> decode(
            const Event &event) const override;
    };

} // namespace antwika::input
