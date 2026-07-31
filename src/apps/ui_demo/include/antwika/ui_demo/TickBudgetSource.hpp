#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::ui_demo
{

    using antwika::event::Event;
    using antwika::replay::IReplaySource;

    /**
     * @brief Adds "this run has gone on long enough" to another source's
     * events, as an engine.stop event.
     *
     * The default gfx backend draws nothing and reports no close, so a
     * showcase left to run until its window was shut would never finish
     * under the build every CI leg produces.
     * apps/gfx3d_demo answers that with a frame cap; this is the same
     * answer for an application whose loop is an EngineLoop rather than
     * a for loop.
     *
     * A cap is deliberately *not* EngineLoop's maxTicks, which throws:
     * reaching the end of a demo is an ordinary end to a run rather than
     * a loop that failed to stop.
     * Saying so as an engine.stop also puts it upstream of the recorder,
     * exactly as poker::WindowCloseSource puts a window close there, so
     * a `--record` run ends its file at the tick it stopped on and
     * replaying that file stops at the same tick.
     */
    class TickBudgetSource final : public IReplaySource
    {
    public:
        /**
         * @brief Construct the source over what it decorates.
         * @param inner Supplies each tick's recorded or scripted events;
         * must outlive this object.
         * @param budget How many ticks to run: the tick with this number
         * is the one carrying the stop, so a budget of zero stops on the
         * first tick.
         */
        TickBudgetSource(IReplaySource &inner, antwika::time::Tick budget);

        TickBudgetSource(const TickBudgetSource &) = delete;
        TickBudgetSource(TickBudgetSource &&) = delete;

        TickBudgetSource &operator=(const TickBudgetSource &) = delete;
        TickBudgetSource &operator=(TickBudgetSource &&) = delete;

        /**
         * @brief Get a tick's events, with a stop once the budget is up.
         * @param tick The tick to fetch events for.
         * @return The inner source's events, plus engine.stop from the
         * budget's tick onwards.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        IReplaySource &inner;
        antwika::time::Tick budget;
    };

} // namespace antwika::ui_demo
