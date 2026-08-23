#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <utility>

#include <antwika/engine/Events.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ISurfaceRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Painter.hpp>

namespace antwika::app
{

    using antwika::event::TickEvent;
    using antwika::gfx::Color;
    using antwika::gfx::ISurfaceRenderer;
    using antwika::gfx::IWindow;
    using antwika::gfx::Size;

    template <typename OverlayT>
    concept HasDrawList = requires(const OverlayT &overlay) {
        {
            overlay.getCommands()
        } -> std::convertible_to<const antwika::ui::DrawList &>;
    };

    [[nodiscard]] inline bool shouldDraw(
        const TickEvent &event, const IWindow &window)
    {
        return event.event.name == antwika::engine::events::kTick
            && window.isOpen();
    }

    template <HasDrawList OverlayT>
    void paintOverlay(
        ISurfaceRenderer &renderer, const OverlayT &overlay)
    {
        antwika::ui::paint(renderer, overlay.getCommands());
    }

    template <HasDrawList OverlayT>
    void paintOverlay(
        ISurfaceRenderer &renderer,
        const std::optional<std::reference_wrapper<const OverlayT>>
            &overlay)
    {
        if (overlay.has_value())
        {
            antwika::ui::paint(renderer, overlay->get().getCommands());
        }
    }

    template <typename FrameT>
    void presentFrame(IWindow &window, FrameT &&frame)
    {
        auto &renderer = window.renderer();

        std::forward<FrameT>(frame)(renderer);
        renderer.present();
    }

    template <typename OverlayT, typename FrameT>
    void presentFrame(
        IWindow &window, const OverlayT &overlay, FrameT &&frame)
    {
        auto &renderer = window.renderer();

        std::forward<FrameT>(frame)(renderer);
        paintOverlay(renderer, overlay);
        renderer.present();
    }

    template <typename OverlayT, typename FrameT>
    void presentViewport(
        IWindow &window,
        const Size canvasSize,
        const Color surroundColor,
        const OverlayT &overlay,
        FrameT &&frame)
    {
        antwika::gfx::ViewportRenderer viewportRenderer(
            window.renderer(), window.getSize(), canvasSize);

        std::forward<FrameT>(frame)(viewportRenderer);
        paintOverlay(viewportRenderer, overlay);

        viewportRenderer.fillLetterbox(surroundColor);
        viewportRenderer.present();
    }

}
