#pragma once

#include <optional>

#include <antwika/event/Event.hpp>

#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    using antwika::event::Event;

    class IInputEventCodec
    {
    public:
        virtual ~IInputEventCodec() = default;

        [[nodiscard]] virtual Event getEncodedEvent(const InputEvent &event) const = 0;

        [[nodiscard]] virtual std::optional<InputEvent> getDecodedEvent(
            const Event &event) const = 0;
    };

}
