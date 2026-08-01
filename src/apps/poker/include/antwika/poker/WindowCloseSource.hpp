#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::poker
{

    using antwika::event::Event;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::IWindow;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Turns closing the window into an engine.stop event.
     *
     * The only route by which a window may influence the engine, and
     * deliberately the same route a replay file takes: it decorates the
     * source the engine loop already pulls its events from, so a close
     * is ordinary external input. That is why antwika::gfx never has to
     * know what an event::Event is, and why a recorded session keeps the
     * close that ended it.
     *
     * The pump lives here rather than beside the drawing because the
     * loop asks a source for a tick's events *before* stepping the
     * engine, so a close seen now stops the session on this tick.
     */
    class WindowCloseSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must
         * outlive this object.
         * @param backend Polled for window events.
         * @param window The window whose close requests count.
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

} // namespace antwika::poker
