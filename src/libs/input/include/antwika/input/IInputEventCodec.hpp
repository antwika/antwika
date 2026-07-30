#pragma once

#include <optional>

#include <antwika/event/Event.hpp>

#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    using antwika::event::Event;

    /**
     * @brief Translates between an InputEvent and the event::Event a
     * replay stores.
     *
     * One type owns both directions, so an encoder and a decoder cannot
     * drift apart into a replay that can be written but not read.
     *
     * An interface with one implementation, so that whatever polls a
     * backend can be tested against a mock codec rather than against the
     * JSON the real one produces.
     */
    class IInputEventCodec
    {
    public:
        virtual ~IInputEventCodec() = default;

        /**
         * @brief Encode an input event as a named, JSON-payloaded event.
         * @param event The edge to encode.
         * @return The event to dispatch, and to persist as-is.
         * @throws InputError If the edge names a key or a button outside
         * its enumeration, which decoding the result would reject too.
         */
        [[nodiscard]] virtual Event encode(const InputEvent &event) const = 0;

        /**
         * @brief Decode an event back into the edge it was encoded from.
         *
         * A caller hands this every event it sees, including events that
         * have nothing to do with input, and acts on what comes back.
         *
         * @param event The event to decode.
         * @return The edge it encodes, or nullopt when the event is not
         * one of this library's.
         * @throws InputError If the event is one of this library's but
         * its payload is not the shape that name requires.
         */
        [[nodiscard]] virtual std::optional<InputEvent> decode(
            const Event &event) const = 0;
    };

} // namespace antwika::input
