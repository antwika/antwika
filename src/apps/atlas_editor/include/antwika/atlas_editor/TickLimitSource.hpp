#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::atlas_editor
{

    using antwika::event::Event;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Ends a session after a given number of ticks, by asking it
     * to stop.
     *
     * **An editor has no end of its own**, and under the default `null`
     * backend nothing else has one either: it reports no window closing
     * and no Escape, so a session there would run until somebody
     * interrupted it -- and a `--record` run that is interrupted never
     * gets as far as writing its file.
     *
     * A stop appended here is ordinary external input, exactly as
     * poker::WindowCloseSource's is: it goes through the source the loop
     * already pulls from, so it is dispatched like any other event and
     * lands in a recording, which is what makes a capped session
     * replayable.
     *
     * That is deliberately not EngineLoop's own `maxTicks`, which
     * *throws* when it is reached -- the right answer for a test with a
     * forgotten stop event, and the wrong one for a session that simply
     * ran as long as it was asked to.
     */
    class TickLimitSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param limit The tick to stop on, or nothing to never stop --
         * which is what somebody in front of a real window asks for.
         */
        TickLimitSource(
            ITickEventSource &inner,
            std::optional<antwika::time::Tick> limit);

        TickLimitSource(const TickLimitSource &) = delete;
        TickLimitSource(TickLimitSource &&) = delete;

        TickLimitSource &operator=(const TickLimitSource &) = delete;
        TickLimitSource &operator=(TickLimitSource &&) = delete;

        /**
         * @brief Get a tick's events, plus a stop once the cap is
         * reached.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events for that tick, followed by
         * engine.stop from the limiting tick onwards.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<antwika::time::Tick> limit;
    };

} // namespace antwika::atlas_editor
