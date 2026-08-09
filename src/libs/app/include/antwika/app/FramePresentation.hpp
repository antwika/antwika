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

    template <typename OverlayT>
    concept Pictured = requires(const OverlayT &overlay) {
        {
            overlay.commands()
        } -> std::convertible_to<const antwika::ui::DrawList &>;
    };

    [[nodiscard]] inline bool drawsOn(
        const TickEvent &event, const IWindow &window)
    {
        return event.event.name == antwika::engine::events::kTick
            && window.isOpen();
    }

    template <Pictured OverlayT>
    void paintOver(IRenderer &renderer, const OverlayT &overlay)
    {
        antwika::ui::paint(renderer, overlay.commands());
    }

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
        paintOver(renderer, overlay);
        renderer.present();
    }

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

}
