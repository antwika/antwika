#pragma once

#include <cstdint>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/SizeF.hpp"

namespace antwika::gfx
{

    struct Viewport final
    {
        Point offsetPoint{};

        std::uint32_t numerator = 1;

        std::uint32_t denominator = 1;

        [[nodiscard]] bool operator==(const Viewport &other) const = default;

        [[nodiscard]] Point toWindow(Point point) const noexcept;

        [[nodiscard]] Rect toWindow(Rect rect) const noexcept;

        [[nodiscard]] PointF toWindow(PointF point) const noexcept;

        [[nodiscard]] RectF toWindow(RectF rect) const noexcept;

        [[nodiscard]] Point toCanvas(Point point) const noexcept;

        [[nodiscard]] std::uint32_t toWindowScale(
            std::uint32_t scale) const noexcept;

        [[nodiscard]] Rect getFrame(Size canvasSize) const noexcept;
    };

    enum class Fit : std::uint8_t
    {
        Stretched,
        IntegerScale,
    };

    [[nodiscard]] Viewport viewportFor(
        Size reportedSize, Size canvasSize, Fit fit = Fit::Stretched) noexcept;

}
