#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::music_editor
{

    using antwika::event::Event;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Adds "this run has gone on long enough" to another source's
     * events, as an engine.stop event.
     *
     * The same answer ui_demo's source gives, and for the same reason:
     * the default gfx backend draws nothing and reports no close, so an
     * editor left to run until its window shut would never finish under
     * the build every CI leg produces.
     *
     * Saying so as an engine.stop puts it upstream of the recorder, so a
     * `--record` run ends its file at the tick it stopped on and
     * replaying that file stops at the same tick.
     */
    class TickBudgetSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the source over what it decorates.
         * @param inner Supplies each tick's events; must outlive this.
         * @param budget How many ticks to run: the tick with this number
         * carries the stop, so a budget of zero stops on the first tick.
         */
        TickBudgetSource(ITickEventSource &inner, time::Tick budget);

        TickBudgetSource(const TickBudgetSource &) = delete;
        TickBudgetSource(TickBudgetSource &&) = delete;

        TickBudgetSource &operator=(const TickBudgetSource &) = delete;
        TickBudgetSource &operator=(TickBudgetSource &&) = delete;

        /**
         * @brief Get one tick's events, plus a stop once time is up.
         * @param tick Which tick.
         * @return What the inner source gave, and maybe an engine.stop.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            time::Tick tick) override;

    private:
        ITickEventSource &inner;
        time::Tick budget;
    };

} // namespace antwika::music_editor
