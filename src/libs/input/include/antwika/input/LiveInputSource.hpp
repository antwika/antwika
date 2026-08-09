#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputBackend.hpp"
#include "antwika/input/IInputEventCodec.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class LiveInputSource final : public ITickEventSource
    {
    public:
        LiveInputSource(
            ITickEventSource &inner,
            IInputBackend &backend,
            const IInputEventCodec &codec);

        LiveInputSource(const LiveInputSource &) = delete;
        LiveInputSource(LiveInputSource &&) = delete;

        LiveInputSource &operator=(const LiveInputSource &) = delete;
        LiveInputSource &operator=(LiveInputSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        IInputBackend &backend;
        const IInputEventCodec &codec;
    };

}
