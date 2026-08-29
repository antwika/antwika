#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <antwika/event/EngineEvents.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"
#include "antwika/input/fakes/SessionSummary.hpp"

namespace antwika::input::fakes
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    class FakeSummarySink final : public ITickEventSink
    {
    public:
        void handle(const TickEvent &event) override
        {
            if (event.event.name == antwika::event::kTick)
            {
                state.beginTick();
                return;
            }

            const auto decodedEvent = codec.getDecodedEvent(event.event);
            if (!decodedEvent.has_value())
            {
                return;
            }

            state.apply(*decodedEvent);

            if (const auto *pressed = std::get_if<KeyPressed>(&*decodedEvent))
            {
                summary.pressedKeys.push_back(
                    std::string(toString(pressed->key)));
            }

            if (std::holds_alternative<PointerButtonPressed>(*decodedEvent))
            {
                ++summary.clicks;
            }

            if (const auto *scroll =
                    std::get_if<PointerScrolled>(&*decodedEvent))
            {
                summary.scrollTotal += scroll->vertical;
            }

            summary.pointerPosition = state.getMouse().getPosition();
            summary.leftHeldAtEnd = state.getMouse().isDown(MouseButton::Left);
        }

        [[nodiscard]] SessionSummary getResult() const
        {
            return summary;
        }

    private:
        InputEventCodec codec;
        InputState state;
        SessionSummary summary;
    };

}
