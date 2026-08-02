#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::IWindow;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Turns closing the window into an engine.stop event, and
     * closes the window itself.
     *
     * The same route into the engine a replay file takes: it decorates
     * the source the loop already pulls its events from, so a close is
     * ordinary external input. That is why antwika::gfx never has to
     * know what an event::Event is, and why a recorded session keeps the
     * close that ended it.
     *
     * The pump lives here rather than beside the drawing because the
     * loop asks a source for a tick's events *before* stepping the
     * engine, so a close seen now stops the session on this tick.
     *
     * **It holds the window rather than an id**, which is the whole
     * difference from simulation::WindowInputSource and is deliberate
     * rather than an oversight.
     * That one holds a gfx::WindowId and so cannot close anything,
     * noting a close request in a bool local to one eventsFor() call.
     * This one calls IWindow::close(), so the window's own open/closed
     * state is what says the session is over -- which is what an
     * application needs when it goes on pumping and rendering *after*
     * the loop has finished, long after a bool inside eventsFor() would
     * have gone out of scope. Holding the final frame up until somebody
     * closes the window is exactly that, and it is why pumpEvents() is
     * public and is called from outside the tick.
     *
     * **So an application on this source owes blog/012 an answer of its
     * own**: the tick carrying the stop still runs to completion, and it
     * must not draw into a window this source has already closed. Its
     * render pass returning early on a closed window is that answer, and
     * simulation::WindowInputSource is the one to reach for when nothing
     * needs the window closed from here, since it needs no such guard.
     */
    class WindowCloseSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param backend Polled for window events; must outlive this
         * object.
         * @param window The window whose close requests count, and the
         * one closed on one; must outlive this object.
         */
        WindowCloseSource(
            ITickEventSource &inner, IGfxBackend &backend, IWindow &window);

        WindowCloseSource(const WindowCloseSource &) = delete;
        WindowCloseSource(WindowCloseSource &&) = delete;

        WindowCloseSource &operator=(const WindowCloseSource &) = delete;
        WindowCloseSource &operator=(WindowCloseSource &&) = delete;

        /**
         * @brief Get a tick's events, plus a stop once the window shut.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events for that tick, followed by
         * engine.stop if the window is no longer open.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

        /**
         * @brief Drain the backend's queue, closing on a close request.
         *
         * Events belonging to another window are ignored, and so is a
         * resize -- the window's size is read afresh on every frame.
         */
        void pumpEvents();

    private:
        ITickEventSource &inner;
        IGfxBackend &backend;
        IWindow &window;
    };

} // namespace antwika::app
