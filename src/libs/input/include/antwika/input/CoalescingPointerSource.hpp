#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class CoalescingPointerSource final : public ITickEventSource
    {
    public:
        explicit CoalescingPointerSource(ITickEventSource &innerSource);

        CoalescingPointerSource(const CoalescingPointerSource &) = delete;
        CoalescingPointerSource(CoalescingPointerSource &&) = delete;

        CoalescingPointerSource &operator=(
            const CoalescingPointerSource &) = delete;
        CoalescingPointerSource &operator=(
            CoalescingPointerSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
    };

}
