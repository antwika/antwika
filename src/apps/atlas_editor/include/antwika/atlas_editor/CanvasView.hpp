#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    inline constexpr std::array<std::uint32_t, 10> kZoomScales{
        1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

    struct CanvasView final
    {
        Point pan{};

        std::size_t zoom = 0;

        [[nodiscard]] bool operator==(const CanvasView &other) const =
            default;
    };

    [[nodiscard]] std::uint32_t scaleOf(CanvasView view) noexcept;

    [[nodiscard]] CanvasView centredView(
        Size canvas, Size image, std::size_t zoom) noexcept;

    [[nodiscard]] CanvasView zoomedIn(CanvasView view, Point anchor) noexcept;

    [[nodiscard]] CanvasView zoomedOut(
        CanvasView view, Point anchor) noexcept;

    [[nodiscard]] CanvasView pannedBy(CanvasView view, Point by) noexcept;

    [[nodiscard]] Pixel pixelAt(CanvasView view, Point point) noexcept;

    [[nodiscard]] Rect pixelRect(CanvasView view, Pixel pixel) noexcept;

    [[nodiscard]] Rect imageRect(CanvasView view, Size image) noexcept;

}
