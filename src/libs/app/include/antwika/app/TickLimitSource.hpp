#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    class TickLimitSource final : public ITickEventSource
    {
    public:
        TickLimitSource(
            ITickEventSource &inner,
            std::optional<antwika::time::Tick> limit);

        TickLimitSource(const TickLimitSource &) = delete;
        TickLimitSource(TickLimitSource &&) = delete;

        TickLimitSource &operator=(const TickLimitSource &) = delete;
        TickLimitSource &operator=(TickLimitSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<antwika::time::Tick> limit;
    };

}
