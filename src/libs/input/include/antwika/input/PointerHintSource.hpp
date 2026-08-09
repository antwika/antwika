#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/PointerHintChannel.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class PointerHintSource final : public ITickEventSource
    {
    public:
        PointerHintSource(
            ITickEventSource &inner,
            const IInputEventCodec &codec,
            PointerHintChannel &channel);

        PointerHintSource(const PointerHintSource &) = delete;
        PointerHintSource(PointerHintSource &&) = delete;

        PointerHintSource &operator=(const PointerHintSource &) = delete;
        PointerHintSource &operator=(PointerHintSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;
        PointerHintChannel &channel;
    };

}
