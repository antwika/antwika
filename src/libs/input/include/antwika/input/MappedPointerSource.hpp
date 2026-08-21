#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/IPointerMapping.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class MappedPointerSource final : public ITickEventSource
    {
    public:
        MappedPointerSource(
            ITickEventSource &innerSource,
            const IInputEventCodec &codec,
            const IPointerMapping &mapping);

        MappedPointerSource(const MappedPointerSource &) = delete;
        MappedPointerSource(MappedPointerSource &&) = delete;

        MappedPointerSource &operator=(const MappedPointerSource &) = delete;
        MappedPointerSource &operator=(MappedPointerSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;
        const IPointerMapping &mapping;
    };

}
