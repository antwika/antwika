#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/InputState.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class IdleMotionSource final : public ITickEventSource
    {
    public:
        IdleMotionSource(
            ITickEventSource &inner, const IInputEventCodec &codec);

        IdleMotionSource(const IdleMotionSource &) = delete;
        IdleMotionSource(IdleMotionSource &&) = delete;

        IdleMotionSource &operator=(const IdleMotionSource &) = delete;
        IdleMotionSource &operator=(IdleMotionSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;

        InputState state;

        std::optional<Event> latched;
    };

}
