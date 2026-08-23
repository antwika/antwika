#include "antwika/gfx/Viewport.hpp"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/PointF.hpp>

namespace antwika::gfx
{

    namespace
    {
        [[nodiscard]] std::int32_t getScaleBy(
            std::int32_t value,
            std::uint32_t numerator,
            std::uint32_t denominator) noexcept
        {
            const auto scaledValue = static_cast<std::int64_t>(value)
                                * static_cast<std::int64_t>(numerator);
            const auto divisor = static_cast<std::int64_t>(denominator);

            const auto quotient = scaledValue / divisor;
            const auto remainder = scaledValue % divisor;

            return static_cast<std::int32_t>(
                remainder < 0 ? quotient - 1 : quotient);
        }
    }

    Point Viewport::toWindow(Point point) const noexcept
    {
        return Point{
            .x = offsetPoint.x + getScaleBy(point.x, numerator, denominator),
            .y = offsetPoint.y + getScaleBy(point.y, numerator, denominator)};
    }

    Rect Viewport::toWindow(Rect rect) const noexcept
    {
        const auto topLeft = toWindow(rect.originPoint);
        const auto bottomRight = toWindow(
            Point{
                .x = rect.originPoint.x
                     + static_cast<std::int32_t>(rect.size.width),
                .y = rect.originPoint.y
                     + static_cast<std::int32_t>(rect.size.height)});

        return Rect{
            .originPoint = topLeft,
            .size = {
                .width = static_cast<std::uint32_t>(
                    bottomRight.x - topLeft.x),
                .height = static_cast<std::uint32_t>(
                    bottomRight.y - topLeft.y)}};
    }

    PointF Viewport::toWindow(const PointF point) const noexcept
    {
        const auto factor = static_cast<float>(numerator)
                            / static_cast<float>(denominator);

        return antwika::gfx::PointF{
            static_cast<float>(offsetPoint.x) + (point.x * factor),
            static_cast<float>(offsetPoint.y) + (point.y * factor)};
    }

    RectF Viewport::toWindow(const RectF rect) const noexcept
    {
        const auto factor = static_cast<float>(numerator)
                            / static_cast<float>(denominator);

        return antwika::gfx::RectF{
            toWindow(rect.originPoint),
            SizeF{rect.size.width * factor, rect.size.height * factor}};
    }

    Point Viewport::toCanvas(Point point) const noexcept
    {
        return Point{
            .x = getScaleBy(point.x - offsetPoint.x, denominator, numerator),
            .y = getScaleBy(point.y - offsetPoint.y, denominator, numerator)};
    }

    std::uint32_t Viewport::toWindowScale(
        std::uint32_t scale) const noexcept
    {
        if (scale == 0)
        {
            return 0;
        }

        const auto scaledValue = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(scale)
            * static_cast<std::uint64_t>(numerator) / denominator);

        return scaledValue == 0 ? 1 : scaledValue;
    }

    Rect Viewport::getFrame(Size canvasSize) const noexcept
    {
        return toWindow(Rect{.originPoint = Point{}, .size = canvasSize});
    }

    Viewport viewportFor(
        Size reportedSize, const Size canvasSize, const Fit fit) noexcept
    {
        if (reportedSize.width == 0 || reportedSize.height == 0
            || canvasSize.width == 0 || canvasSize.height == 0)
        {
            return Viewport{};
        }

        Viewport viewport{
            .numerator = reportedSize.height, .denominator = canvasSize.height};

        if (static_cast<std::uint64_t>(reportedSize.width) * canvasSize.height
            < static_cast<std::uint64_t>(reportedSize.height)
                  * canvasSize.width)
        {
            viewport.numerator = reportedSize.width;
            viewport.denominator = canvasSize.width;
        }

        const auto common =
            std::gcd(viewport.numerator, viewport.denominator);

        viewport.numerator /= common;
        viewport.denominator /= common;

        if (fit == Fit::IntegerScale)
        {
            viewport.numerator =
                std::max(1U, viewport.numerator / viewport.denominator);
            viewport.denominator = 1;
        }

        const auto usedSize = viewport.getFrame(canvasSize).size;

        viewport.offsetPoint = Point{
            .x = static_cast<std::int32_t>(
                (reportedSize.width - usedSize.width) / 2),
            .y = static_cast<std::int32_t>(
                (reportedSize.height - usedSize.height) / 2)};

        return viewport;
    }

}
