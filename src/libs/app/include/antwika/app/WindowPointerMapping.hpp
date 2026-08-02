#pragma once

#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IPointerMapping.hpp>
#include <antwika/input/Position.hpp>

namespace antwika::app
{

    using antwika::gfx::IWindow;
    using antwika::gfx::Size;
    using antwika::input::IPointerMapping;
    using antwika::input::Position;

    /**
     * @brief Reads a pointer position in window pixels as one on the
     * fixed canvas an application draws and hit-tests against.
     *
     * The other half of gfx::ViewportRenderer, and the reason both are
     * safe: the renderer places the canvas inside the drawable area
     * through gfx::viewportFor(), and this runs the very same transform
     * backwards. So a click lands on whatever the pointer is over,
     * whatever size the window is, and the coordinate that reaches the
     * application is the canvas one either way.
     *
     * **It belongs to antwika::app because nowhere lower may name both
     * ends.** antwika::input does not depend on antwika::gfx and cannot
     * name a window; antwika::gfx does not depend on antwika::input and
     * cannot name a pointer. This is the sentence saying the two
     * describe one thing, next to app::asPoint(), which says the same
     * about the types.
     *
     * **What it hands back is what a recording will hold**, because
     * input::InputPipeline puts it upstream of the recorder. That is the
     * whole design: a session recorded on a window of one size replays
     * on a window of any other, with no window geometry in the file and
     * nothing in the tick path aware that a window has a size at all.
     *
     * The reported size is read afresh on every call rather than
     * captured, so somebody dragging an edge mid-session is handled by
     * the next click being mapped through the new size -- and by nothing
     * else, since no layout and no hit test ever learned the number.
     *
     * The one thing it cannot correct for is a backend reporting a
     * pointer in coordinates its window does not report a size in, which
     * a high-DPI display can produce. Both are the window system's
     * pixels on every configuration this project builds for.
     */
    class WindowPointerMapping final : public IPointerMapping
    {
    public:
        /**
         * @brief Construct the mapping over the window it reads.
         * @param window The window whose reported size places the
         * canvas; must outlive this object.
         * @param canvas The fixed size the application lays out
         * against.
         */
        WindowPointerMapping(const IWindow &window, Size canvas);

        WindowPointerMapping(const WindowPointerMapping &) = delete;
        WindowPointerMapping(WindowPointerMapping &&) = delete;

        WindowPointerMapping &operator=(
            const WindowPointerMapping &) = delete;
        WindowPointerMapping &operator=(WindowPointerMapping &&) = delete;

        /**
         * @brief Read a window position as a canvas position.
         * @param position Where the device said the pointer was.
         * @return The same place, in canvas pixels; outside the canvas
         * when the pointer was over a letterboxed bar.
         */
        [[nodiscard]] Position toSurface(
            Position position) const override;

    private:
        const IWindow &window;
        Size canvas;
    };

} // namespace antwika::app
