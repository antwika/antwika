#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputBackend.hpp"
#include "antwika/input/IInputEventCodec.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Adds whatever a keyboard and a pointer did to another
     * source's events, encoded as input.* events.
     *
     * This is the only way live input reaches the engine. ITickEventSource is
     * the seam the tick loop already asks once per tick, so input arriving
     * through it is recorded by the same TickEventRecorder that records
     * everything else, and a `--record` run therefore replays to the same
     * state with no extra machinery. Adding a second entry point is what
     * would break that, so there is not one.
     *
     * A decorator rather than a source of its own, so an application can
     * seed itself from a scripted file and still take live input, the same
     * shape antwika::life::WindowInputSource uses to add a window close.
     * The inner source's events come first each tick, so ordering within a
     * tick does not depend on how fast a device was polled.
     *
     * What it deliberately does not do is translate an edge into
     * application meaning. A click becomes "toggle the cell at (3, 4)"
     * downstream of the recorder, in a tick sink, so that the replay
     * stores the click and regenerates the toggle -- storing the toggle
     * instead would persist a derived event, and a replay is meant to hold
     * only what came from outside.
     *
     * An application replaying a file must not attach this, or the
     * recorded input arrives alongside the live input and every event
     * happens twice.
     */
    class LiveInputSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the source over what it decorates and polls.
         * @param inner Supplies each tick's recorded or scripted events.
         * Must outlive this source.
         * @param backend Polled dry once per tick. Must outlive this
         * source.
         * @param codec Encodes each edge. Must outlive this source.
         */
        LiveInputSource(
            ITickEventSource &inner,
            IInputBackend &backend,
            const IInputEventCodec &codec);

        LiveInputSource(const LiveInputSource &) = delete;
        LiveInputSource(LiveInputSource &&) = delete;

        LiveInputSource &operator=(const LiveInputSource &) = delete;
        LiveInputSource &operator=(LiveInputSource &&) = delete;

        /**
         * @brief Get the events that occurred on a given tick.
         *
         * Drains the backend every tick, whether or not the application
         * has any use for what it finds, because a queue nobody empties
         * is a device that eventually stops being heard.
         *
         * @param tick The tick to fetch events for.
         * @return The inner source's events for that tick, followed by
         * one event per edge the backend reported since the last tick.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        IInputBackend &backend;
        const IInputEventCodec &codec;
    };

} // namespace antwika::input
