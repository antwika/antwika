#include "antwika/atlas_editor/CanvasView.hpp"

#include <cstddef>
#include <cstdint>

namespace antwika::atlas_editor
{

    namespace
    {
        // Division that rounds towards minus infinity.
        // Truncation maps both -1 and 0 onto image pixel 0.
        // That is one column twice as wide as every other one.
        // apps/game's floorDiv() exists for the same reason.
        [[nodiscard]] std::int32_t floorDiv(
            const std::int32_t value, const std::uint32_t by) noexcept
        {
            const auto divisor = static_cast<std::int32_t>(by);
            const std::int32_t quotient = value / divisor;

            if (value % divisor != 0 && value < 0)
            {
                return quotient - 1;
            }

            return quotient;
        }

        // Worked out in 64 bits and narrowed once.
        // A far pan times a zoom of eight is nowhere near an int32.
        // The multiplication is the one place that could change that.
        [[nodiscard]] std::int32_t scaled(
            const std::int32_t value, const std::uint32_t by) noexcept
        {
            return static_cast<std::int32_t>(
                static_cast<std::int64_t>(value)
                * static_cast<std::int64_t>(by));
        }

        [[nodiscard]] CanvasView zoomedTo(
            const CanvasView view,
            const std::size_t zoom,
            const Point anchor) noexcept
        {
            // The pixel under the anchor is the same one after.
            // That is what "keeping it put" means.
            // The pan is whatever puts it back under that point.
            const Pixel under = pixelAt(view, anchor);
            const CanvasView zoomed{.pan = view.pan, .zoom = zoom};
            const std::uint32_t scale = scaleOf(zoomed);

            return CanvasView{
                .pan = {
                    .x = anchor.x - scaled(under.x, scale),
                    .y = anchor.y - scaled(under.y, scale)},
                .zoom = zoom};
        }
    } // namespace

    std::uint32_t scaleOf(const CanvasView view) noexcept
    {
        if (view.zoom >= kZoomScales.size())
        {
            return kZoomScales.back();
        }

        return kZoomScales[view.zoom];
    }

    CanvasView centredView(
        const Size canvas, const Size image, const std::size_t zoom) noexcept
    {
        const CanvasView at{.pan = {}, .zoom = zoom};
        const std::uint32_t scale = scaleOf(at);

        // Signed throughout, deliberately.
        // An image wider than the canvas belongs at a negative pan.
        // Unsigned arithmetic would wrap to four billion instead.
        const auto width = static_cast<std::int64_t>(image.width) * scale;
        const auto height =
            static_cast<std::int64_t>(image.height) * scale;

        return CanvasView{
            .pan = {
                .x = static_cast<std::int32_t>(
                    (static_cast<std::int64_t>(canvas.width) - width) / 2),
                .y = static_cast<std::int32_t>(
                    (static_cast<std::int64_t>(canvas.height) - height)
                    / 2)},
            .zoom = zoom};
    }

    CanvasView zoomedIn(const CanvasView view, const Point anchor) noexcept
    {
        if (view.zoom + 1 >= kZoomScales.size())
        {
            return view;
        }

        return zoomedTo(view, view.zoom + 1, anchor);
    }

    CanvasView zoomedOut(const CanvasView view, const Point anchor) noexcept
    {
        if (view.zoom == 0)
        {
            return view;
        }

        return zoomedTo(view, view.zoom - 1, anchor);
    }

    CanvasView pannedBy(const CanvasView view, const Point by) noexcept
    {
        return CanvasView{
            .pan = {.x = view.pan.x + by.x, .y = view.pan.y + by.y},
            .zoom = view.zoom};
    }

    Pixel pixelAt(const CanvasView view, const Point point) noexcept
    {
        const std::uint32_t scale = scaleOf(view);

        return Pixel{
            .x = floorDiv(point.x - view.pan.x, scale),
            .y = floorDiv(point.y - view.pan.y, scale)};
    }

    Rect pixelRect(const CanvasView view, const Pixel pixel) noexcept
    {
        const std::uint32_t scale = scaleOf(view);

        return Rect{
            .origin = {
                .x = view.pan.x + scaled(pixel.x, scale),
                .y = view.pan.y + scaled(pixel.y, scale)},
            .size = {.width = scale, .height = scale}};
    }

    Rect imageRect(const CanvasView view, const Size image) noexcept
    {
        const std::uint32_t scale = scaleOf(view);

        return Rect{
            .origin = view.pan,
            .size = {
                .width = image.width * scale,
                .height = image.height * scale}};
    }

} // namespace antwika::atlas_editor
