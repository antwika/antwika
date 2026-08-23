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

        [[nodiscard]] virtual Event getEncode(const InputEvent &event) const = 0;

        [[nodiscard]] virtual std::optional<InputEvent> getDecode(
            const Event &event) const = 0;
    };

}
