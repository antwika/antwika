#include "antwika/atlas_editor/Preview.hpp"

#include <algorithm>
#include <cstddef>

#include <antwika/gfx/Point.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Point;

    namespace
    {
        [[nodiscard]] std::int32_t rightOf(const Rect &rect) noexcept
        {
            return rect.origin.x
                   + static_cast<std::int32_t>(rect.size.width);
        }

        [[nodiscard]] std::int32_t bottomOf(const Rect &rect) noexcept
        {
            return rect.origin.y
                   + static_cast<std::int32_t>(rect.size.height);
        }

        [[nodiscard]] std::optional<Rect> overlap(
            const Rect &left, const Rect &right) noexcept
        {
            const auto x = std::max(left.origin.x, right.origin.x);
            const auto y = std::max(left.origin.y, right.origin.y);
            const auto to = std::min(rightOf(left), rightOf(right));
            const auto floor = std::min(bottomOf(left), bottomOf(right));

            if (to <= x || floor <= y)
            {
                return std::nullopt;
            }

            return Rect{
                .origin = {.x = x, .y = y},
                .size = {
                    .width = static_cast<std::uint32_t>(to - x),
                    .height = static_cast<std::uint32_t>(floor - y)}};
        }

        [[nodiscard]] std::int32_t floorOver(
            const std::int32_t value, const std::uint32_t by) noexcept
        {
            const auto divisor = static_cast<std::int32_t>(by);
            const auto quotient = value / divisor;

            return value % divisor != 0 && value < 0 ? quotient - 1
                                                     : quotient;
        }

        [[nodiscard]] std::int32_t ceilOver(
            const std::int32_t value, const std::uint32_t by) noexcept
        {
            return -floorOver(-value, by);
        }

        [[nodiscard]] std::uint32_t fittingZoom(
            const Rect &pane, const Rect &slot) noexcept
        {
            for (auto zoom = kZoomScales.size(); zoom-- > 0;)
            {
                const auto scale = kZoomScales[zoom];

                if (slot.size.width * scale <= pane.size.width
                    && slot.size.height * scale <= pane.size.height)
                {
                    return static_cast<std::uint32_t>(zoom);
                }
            }

            return 0;
        }
    }

    bool paneHolds(const Rect pane, const Point at) noexcept
    {
        return at.x >= pane.origin.x && at.x < rightOf(pane)
               && at.y >= pane.origin.y && at.y < bottomOf(pane);
    }

    std::optional<Blit> blitFor(
        const CanvasView view, const Rect pane, const Size image) noexcept
    {
        const auto drawn = overlap(pane, imageRect(view, image));

        if (!drawn.has_value())
        {
            return std::nullopt;
        }

        const auto scale = scaleOf(view);

        const Pixel first{
            .x = ceilOver(drawn->origin.x - view.pan.x, scale),
            .y = ceilOver(drawn->origin.y - view.pan.y, scale)};

        const Pixel last{
            .x = floorOver(rightOf(*drawn) - view.pan.x, scale) - 1,
            .y = floorOver(bottomOf(*drawn) - view.pan.y, scale) - 1};

        if (last.x < first.x || last.y < first.y)
        {
            return std::nullopt;
        }

        const Rect source{
            .origin = {.x = first.x, .y = first.y},
            .size = {
                .width = static_cast<std::uint32_t>(last.x - first.x) + 1,
                .height =
                    static_cast<std::uint32_t>(last.y - first.y) + 1}};

        return Blit{
            .source = source,
            .destination = Rect{
                .origin = pixelRect(view, first).origin,
                .size = {
                    .width = source.size.width * scale,
                    .height = source.size.height * scale}}};
    }

    CanvasView fittedView(const Rect pane, const Rect slot) noexcept
    {
        const auto zoom = fittingZoom(pane, slot);
        const auto scale = kZoomScales[zoom];

        const auto width =
            static_cast<std::int64_t>(slot.size.width) * scale;
        const auto height =
            static_cast<std::int64_t>(slot.size.height) * scale;

        return CanvasView{
            .pan = {
                .x = static_cast<std::int32_t>(
                    pane.origin.x
                    + (static_cast<std::int64_t>(pane.size.width) - width)
                          / 2
                    - static_cast<std::int64_t>(slot.origin.x) * scale),
                .y = static_cast<std::int32_t>(
                    pane.origin.y
                    + (static_cast<std::int64_t>(pane.size.height)
                       - height)
                          / 2
                    - static_cast<std::int64_t>(slot.origin.y) * scale)},
            .zoom = zoom};
    }

    std::optional<CanvasView> viewOfSlot(
        const Rect pane,
        const TileGrid tiles,
        const Size image,
        const std::uint32_t slot) noexcept
    {
        const auto across = columnsIn(tiles, image);

        if (across == 0 || slot >= across * rowsIn(tiles, image))
        {
            return std::nullopt;
        }

        return fittedView(
            pane,
            Rect{
                .origin = {
                    .x = static_cast<std::int32_t>(
                        (slot % across) * tiles.width),
                    .y = static_cast<std::int32_t>(
                        (slot / across) * tiles.height)},
                .size = {.width = tiles.width, .height = tiles.height}});
    }

}
