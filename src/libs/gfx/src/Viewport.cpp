#include "antwika/gfx/Viewport.hpp"

#include <cstdint>
#include <numeric>

namespace antwika::gfx
{

    namespace
    {
        // Widened before multiplying, never after.
        // A coordinate times a window height overflows 32 bits early.
        //
        // Rounded down rather than towards zero, unlike C++'s division.
        // toCanvas() divides a distance from the offset.
        // That distance is negative all along the left and top bars.
        // Truncation maps the last scale - 1 pixels of it onto 0.
        // Which is a coordinate on the canvas.
        // WindowPointerMapping feeds the recorder.
        // So that is a click in the bar recorded on the first column.
        // It can activate whatever widget the layout put at the edge.
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
    } // namespace

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

    Point Viewport::toCanvas(Point point) const noexcept
    {
        return Point{
            .x = scaleBy(point.x - offset.x, denominator, numerator),
            .y = scaleBy(point.y - offset.y, denominator, numerator)};
    }

    std::uint32_t Viewport::toWindowScale(
        std::uint32_t scale) const noexcept
    {
        // A caller asking for nothing still gets nothing.
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

        // The one case honouring the height would draw off the edges.
        // Compared as a cross product, so neither ratio is rounded.
        if (static_cast<std::uint64_t>(reported.width) * canvas.height
            < static_cast<std::uint64_t>(reported.height) * canvas.width)
        {
            viewport.numerator = reported.width;
            viewport.denominator = canvas.width;
        }

        // In lowest terms, so one scale is one value.
        // A window matching the canvas is then the identity exactly.
        // Which is what every headless run has to compare equal to.
        const auto common =
            std::gcd(viewport.numerator, viewport.denominator);

        viewport.numerator /= common;
        viewport.denominator /= common;

        // Worked out before the offset is, and independent of it.
        // A size is the difference of two transformed corners.
        const auto used = viewport.frame(canvas).size;

        viewport.offset = Point{
            .x = static_cast<std::int32_t>(
                (reported.width - used.width) / 2),
            .y = static_cast<std::int32_t>(
                (reported.height - used.height) / 2)};

        return viewport;
    }

} // namespace antwika::gfx
