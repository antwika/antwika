#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::life
{

    using antwika::event::Event;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::WindowId;
    using antwika::replay::IReplaySource;

    /**
     * @brief Adds "the window was closed" to another source's events, as an
     * engine.stop event.
     *
     * This is the whole reason antwika::gfx knows nothing about
     * antwika::event: turning a gfx::WindowEvent into an
     * antwika::event::Event is the application's job, so a window reaches
     * the engine through IReplaySource like any other external input. That
     * makes closing a window recordable, and therefore replayable -- a
     * `--record` run stops at the tick it was closed on, and replaying that
     * file stops at the same tick.
     *
     * It holds a window's id rather than the window itself, so it cannot
     * close anything. Closing the window here would leave the tick that
     * carries the stop event drawing into a closed window, since that tick
     * still runs to completion.
     *
     * Two limitations worth knowing:
     *
     * It sits in front of a `--replay` run too, so closing the window part
     * way through a replay injects a stop the file does not contain and
     * ends the run early. That is the right behaviour for someone closing a
     * window, but it means a replay reproduces a recorded run exactly only
     * if it is left to finish.
     *
     * It also discards events belonging to other windows, off a queue that
     * serves all of a backend's windows. A second consumer of the same
     * backend would find its events already taken.
     */
    class WindowInputSource final : public IReplaySource
    {
    public:
        /**
         * @brief Construct the source over what it decorates and observes.
         * @param inner Supplies each tick's recorded or scripted events.
         * Must outlive this source.
         * @param backend Polled once per tick for window events.
         * Must outlive this source.
         * @param window Id of the window whose close requests count.
         */
        WindowInputSource(
            IReplaySource &inner, IGfxBackend &backend, WindowId window);

        WindowInputSource(const WindowInputSource &) = delete;
        WindowInputSource(WindowInputSource &&) = delete;

        WindowInputSource &operator=(const WindowInputSource &) = delete;
        WindowInputSource &operator=(WindowInputSource &&) = delete;

        /**
         * @brief Get the events that occurred on a given tick.
         *
         * Drains the backend's event queue every tick, whether or not
         * anything in it matters, because a queue nobody empties is a
         * window nobody can close.
         *
         * @param tick The tick to fetch events for.
         * @return The inner source's events for that tick, followed by one
         * engine.stop if this window was asked to close.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        IReplaySource &inner;
        IGfxBackend &backend;
        WindowId window;
    };

} // namespace antwika::life
