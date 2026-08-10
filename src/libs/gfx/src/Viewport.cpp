#include "antwika/gfx/Viewport.hpp"

#include <cstdint>
#include <numeric>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/PointF.hpp>

namespace antwika::gfx
{

    namespace
    {
        [[nodiscard]] std::int32_t scaleBy(
            std::int32_t value,
            std::uint32_t numerator,
            std::uint32_t denominator) noexcept
        {
            const auto scaled = static_cast<std::int64_t>(value)
                                * static_cast<std::int64_t>(numerator);
            const auto by = static_cast<std::int64_t>(denominator);

            const auto quotient = scaled / by;
            const auto remainder = scaled % by;

            return static_cast<std::int32_t>(
                remainder < 0 ? quotient - 1 : quotient);
        }
    }

    Point Viewport::toWindow(Point point) const noexcept
    {
        return Point{
            .x = offset.x + scaleBy(point.x, numerator, denominator),
            .y = offset.y + scaleBy(point.y, numerator, denominator)};
    }

    Rect Viewport::toWindow(Rect rect) const noexcept
    {
        const auto topLeft = toWindow(rect.origin);
        const auto bottomRight = toWindow(
            Point{
                .x = rect.origin.x
                     + static_cast<std::int32_t>(rect.size.width),
                .y = rect.origin.y
                     + static_cast<std::int32_t>(rect.size.height)});

        return Rect{
            .origin = topLeft,
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
            static_cast<float>(offset.x) + (point.x * factor),
            static_cast<float>(offset.y) + (point.y * factor)};
    }

    RectF Viewport::toWindow(const RectF rect) const noexcept
    {
        const auto factor = static_cast<float>(numerator)
                            / static_cast<float>(denominator);

        return antwika::gfx::RectF{
            toWindow(rect.origin),
            SizeF{rect.size.width * factor, rect.size.height * factor}};
    }

    Point Viewport::toCanvas(Point point) const noexcept
    {
        return Point{
            .x = scaleBy(point.x - offset.x, denominator, numerator),
            .y = scaleBy(point.y - offset.y, denominator, numerator)};
    }

    std::uint32_t Viewport::toWindowScale(
        std::uint32_t scale) const noexcept
    {
        if (scale == 0)
        {
            return 0;
        }

        const auto scaled = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(scale)
            * static_cast<std::uint64_t>(numerator) / denominator);

        return scaled == 0 ? 1 : scaled;
    }

    Rect Viewport::frame(Size canvas) const noexcept
    {
        return toWindow(Rect{.origin = Point{}, .size = canvas});
    }

    Viewport viewportFor(Size reported, Size canvas) noexcept
    {
        if (reported.width == 0 || reported.height == 0
            || canvas.width == 0 || canvas.height == 0)
        {
            return Viewport{};
        }

        Viewport viewport{
            .numerator = reported.height, .denominator = canvas.height};

        if (static_cast<std::uint64_t>(reported.width) * canvas.height
            < static_cast<std::uint64_t>(reported.height) * canvas.width)
        {
            viewport.numerator = reported.width;
            viewport.denominator = canvas.width;
        }

        const auto common =
            std::gcd(viewport.numerator, viewport.denominator);

        viewport.numerator /= common;
        viewport.denominator /= common;

        const auto used = viewport.frame(canvas).size;

        viewport.offset = Point{
            .x = static_cast<std::int32_t>(
                (reported.width - used.width) / 2),
            .y = static_cast<std::int32_t>(
                (reported.height - used.height) / 2)};

        return viewport;
    }

}
