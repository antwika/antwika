#pragma once

#include <optional>

#include <antwika/event/Event.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    class InputEventCodec final : public IInputEventCodec
    {
    public:
        [[nodiscard]] Event getEncode(const InputEvent &event) const override;

        [[nodiscard]] std::optional<InputEvent> getDecode(
            const Event &event) const override;
    };

}
