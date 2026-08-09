#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IFramePump.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class BufferedInputSource final : public ITickEventSource,
                                      public IFramePump
    {
    public:
        explicit BufferedInputSource(ITickEventSource &inner);

        BufferedInputSource(const BufferedInputSource &) = delete;
        BufferedInputSource(BufferedInputSource &&) = delete;

        BufferedInputSource &operator=(const BufferedInputSource &) = delete;
        BufferedInputSource &operator=(BufferedInputSource &&) = delete;

        void pump(antwika::time::Tick tick) override;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::vector<Event> pending;
    };

}
