#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/Key.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class StopOnKeySource final : public ITickEventSource
    {
    public:
        StopOnKeySource(
            ITickEventSource &innerSource,
            const IInputEventCodec &codec,
            Key key);

        StopOnKeySource(const StopOnKeySource &) = delete;
        StopOnKeySource(StopOnKeySource &&) = delete;

        StopOnKeySource &operator=(const StopOnKeySource &) = delete;
        StopOnKeySource &operator=(StopOnKeySource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;
        Key key;
    };

}
