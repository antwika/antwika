#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <utility>

#include <antwika/engine/Events.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Painter.hpp>

namespace antwika::app
{

    using antwika::event::TickEvent;
    using antwika::gfx::Color;
    using antwika::gfx::IRenderer;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;

    /**
     * @brief Anything holding a picture somebody else described.
     *
     * A console's sheet, a toolbar, a score bar: whatever was described
     * inside the tick path and is only painted here.
     * Stated as a concept rather than as an interface so no application
     * type has to inherit anything to be painted, and so this header
     * names neither antwika::console nor any app's own overlay.
     */
    template <typename OverlayT>
    concept Pictured = requires(const OverlayT &overlay) {
        {
            overlay.commands()
        } -> std::convertible_to<const antwika::ui::DrawList &>;
    };

    /**
     * @brief Whether this event draws a frame into this window.
     *
     * The guard every render sink opens with: a frame is drawn on the
     * engine's own tick and on nothing else, and never into a window
     * somebody has closed.
     * A sink asks the window this and asks it nothing else -- it never
     * closes one, which is what keeps rendering a write-only projection
     * (see blog/012).
     *
     * @param event The event a sink was handed.
     * @param window The window that frame would go to.
     * @return True when this is a tick and the window is still open.
     */
    [[nodiscard]] inline bool drawsOn(
        const TickEvent &event, const IWindow &window)
    {
        return event.event.name == antwika::engine::events::kTick
            && window.isOpen();
    }

    /**
     * @brief Paint an overlay's picture over whatever is already drawn.
     *
     * @param renderer Receives one call per command.
     * @param overlay Holds the picture, described elsewhere.
     */
    template <Pictured OverlayT>
    void paintOver(IRenderer &renderer, const OverlayT &overlay)
    {
        antwika::ui::paint(renderer, overlay.commands());
    }

    /**
     * @brief Paint an overlay that may not be mounted at all.
     *
     * The optional half of the pair, so that an application holding a
     * console only when one was asked for reads the same as one always
     * holding it, and an absent overlay paints nothing.
     *
     * @param renderer Receives one call per command.
     * @param overlay The overlay, or nothing.
     */
    template <Pictured OverlayT>
    void paintOver(
        IRenderer &renderer,
        const std::optional<std::reference_wrapper<const OverlayT>>
            &overlay)
    {
        if (overlay.has_value())
        {
            antwika::ui::paint(renderer, overlay->get().commands());
        }
    }

    /**
     * @brief Draw one frame into a window's renderer and present it.
     *
     * The un-paced cousin of FramePacedSource: one frame, drawn where
     * the caller already is rather than in the gap before a tick's
     * events are read.
     * The frame body is handed the renderer and nothing else, on
     * IFramePass::draw()'s terms -- there is nothing here it could read
     * a world from or write one to.
     *
     * @param window Whose renderer receives the frame.
     * @param frame Draws the picture; called with the renderer.
     */
    template <typename FrameT>
    void presentFrame(IWindow &window, FrameT &&frame)
    {
        auto &renderer = window.renderer();

        std::forward<FrameT>(frame)(renderer);
        renderer.present();
    }

    /**
     * @brief Draw one frame, paint an overlay last, and present it.
     *
     * The overlay goes on after the frame body has finished, which is
     * how the console's sheet stands over everything an application
     * draws whatever else that application paints on the way.
     * Owning the order here is the point: it is one rule rather than
     * ten call sites that each have to remember it.
     *
     * @param window Whose renderer receives the frame.
     * @param overlay Painted last; a plain overlay or an optional one.
     * @param frame Draws the picture; called with the renderer.
     */
    template <typename OverlayT, typename FrameT>
    void presentFrame(
        IWindow &window, const OverlayT &overlay, FrameT &&frame)
    {
        auto &renderer = window.renderer();

        std::forward<FrameT>(frame)(renderer);
        paintOver(renderer, overlay);
        renderer.present();
    }

    /**
     * @brief Draw one frame through a viewport, and present it.
     *
     * **The one place an application's rendering reads the size a
     * window reports**, and it reads it to place a picture and nothing
     * else: every call the frame body makes is in canvas pixels, and
     * the gfx::ViewportRenderer built fresh here scales and centres
     * them into whatever the window currently is.
     * A new one each frame, so a resize and a fullscreen toggle need no
     * handling of their own -- see docs/resizable-windows.md.
     *
     * The surround is filled after the picture rather than before it,
     * so whatever reached past the canvas's edge is covered rather than
     * left showing in the bars.
     *
     * @param window Whose renderer receives the frame.
     * @param canvas The fixed size every drawn command is expressed in.
     * @param surround Fills whatever the canvas does not cover.
     * @param overlay Painted last before the surround; plain or
     * optional.
     * @param frame Draws the picture; called with the viewport.
     */
    template <typename OverlayT, typename FrameT>
    void presentViewport(
        IWindow &window,
        const Size canvas,
        const Color surround,
        const OverlayT &overlay,
        FrameT &&frame)
    {
        antwika::gfx::ViewportRenderer view(
            window.renderer(), window.size(), canvas);

        std::forward<FrameT>(frame)(view);
        paintOver(view, overlay);

        view.fillSurround(surround);
        view.present();
    }

} // namespace antwika::app
