#pragma once

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/WindowId.hpp>

namespace antwika::app
{

    using antwika::gfx::IGfxBackend;
    using antwika::gfx::WindowId;

    /**
     * @brief Drain a backend's event queue and say whether one window
     * was asked to close.
     *
     * A backend pumps one queue for all of its windows, so every caller
     * reading it has to do the same two things: take everything that is
     * there, and ignore whatever belongs to somebody else's window.
     * Four places were doing that by hand -- WindowCloseSource,
     * WindowInputSource and the two demos' frame loops -- and a queue
     * that is drained a little differently in one of them is a window
     * that stops answering rather than an obvious mistake.
     *
     * **The queue is drained whether or not this window is named.**
     * Leaving another window's events in it would grow it without
     * bound in a process nobody closes.
     *
     * This says what was asked rather than acting on it: a caller
     * holding an IWindow closes it, and one holding only an id emits
     * engine.stop instead, which is exactly the difference between the
     * two sources here.
     *
     * @param backend The backend whose queue to drain.
     * @param window The window whose close requests count.
     * @return True when at least one close was asked of that window.
     */
    [[nodiscard]] bool closeRequestedOn(
        IGfxBackend &backend, WindowId window);

} // namespace antwika::app
