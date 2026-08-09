#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input::fakes
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    struct SessionSummary final
    {
        Position pointer;
        std::vector<std::string> pressedKeys;
        std::uint32_t clicks = 0;
        std::int32_t scrolled = 0;
        bool leftHeldAtEnd = false;

        [[nodiscard]] bool operator==(
            const SessionSummary &other) const = default;
    };

    class FakeSummarySink final : public ITickEventSink
    {
    public:
        void handle(const TickEvent &event) override
        {
            if (event.event.name == antwika::engine::events::kTick)
            {
                state.beginTick();
                return;
            }

            const auto decoded = codec.decode(event.event);
            if (!decoded.has_value())
            {
                return;
            }

            state.apply(*decoded);

            if (const auto *pressed = std::get_if<KeyPressed>(&*decoded))
            {
                summary.pressedKeys.push_back(
                    std::string(toString(pressed->key)));
            }

            if (std::holds_alternative<PointerButtonPressed>(*decoded))
            {
                ++summary.clicks;
            }

            if (const auto *scroll =
                    std::get_if<PointerScrolled>(&*decoded))
            {
                summary.scrolled += scroll->vertical;
            }

            summary.pointer = state.mouse().position();
            summary.leftHeldAtEnd = state.mouse().isDown(MouseButton::Left);
        }

        [[nodiscard]] SessionSummary result() const
        {
            return summary;
        }

    private:
        InputEventCodec codec;
        InputState state;
        SessionSummary summary;
    };

}
