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

    class IdleMotionFilter final : public ITickEventSource
    {
    public:
        IdleMotionFilter(
            ITickEventSource &innerSource, const IInputEventCodec &codec);

        IdleMotionFilter(const IdleMotionFilter &) = delete;
        IdleMotionFilter(IdleMotionFilter &&) = delete;

        IdleMotionFilter &operator=(const IdleMotionFilter &) = delete;
        IdleMotionFilter &operator=(IdleMotionFilter &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;

        InputState state;

        std::optional<Event> heldMotionEvent;
    };

}
