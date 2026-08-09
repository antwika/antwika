#include "antwika/atlas_editor/Shapes.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        [[nodiscard]] bool holds(const Size within, const Pixel pixel)
        {
            return pixel.x >= 0 && pixel.y >= 0
                   && pixel.x < static_cast<std::int32_t>(within.width)
                   && pixel.y < static_cast<std::int32_t>(within.height);
        }

        struct Bounds final
        {
            std::int64_t low = 0;

            std::int64_t high = 0;

            std::int64_t span = 0;
        };

        [[nodiscard]] Bounds boundsOf(
            const std::int32_t one, const std::int32_t other)
        {
            const auto low = std::min(one, other);
            const auto high = std::max(one, other);

            return Bounds{
                .low = low, .high = high, .span = high - low + 1};
        }

        [[nodiscard]] bool insideEllipse(
            const std::int64_t x,
            const std::int64_t y,
            const Bounds &across,
            const Bounds &down)
        {
            const auto dx = 2 * x - across.low - across.high;
            const auto dy = 2 * y - down.low - down.high;

            const auto width = across.span;
            const auto height = down.span;

            return dx * dx * height * height + dy * dy * width * width
                   <= width * width * height * height;
        }

        [[nodiscard]] bool ringsEllipse(
            const std::int64_t x,
            const std::int64_t y,
            const Bounds &across,
            const Bounds &down)
        {
            if (!insideEllipse(x, y, across, down))
            {
                return false;
            }

            return !insideEllipse(x - 1, y, across, down)
                   || !insideEllipse(x + 1, y, across, down)
                   || !insideEllipse(x, y - 1, across, down)
                   || !insideEllipse(x, y + 1, across, down);
        }
    }

    std::vector<Pixel> linePixels(
        const Pixel from, const Pixel to, const Size within)
    {
        const std::int32_t stepX = to.x < from.x ? -1 : 1;
        const std::int32_t stepY = to.y < from.y ? -1 : 1;

        const std::int32_t spanX = (to.x - from.x) * stepX;
        const std::int32_t spanY = (from.y - to.y) * stepY;

        std::int32_t error = spanX + spanY;
        Pixel walked = from;

        std::vector<Pixel> covered;

        for (;;)
        {
            if (holds(within, walked))
            {
                covered.push_back(walked);
            }

            if (walked.x == to.x && walked.y == to.y)
            {
                return covered;
            }

            const std::int32_t doubled = 2 * error;

            if (doubled >= spanY)
            {
                error += spanY;
                walked.x += stepX;
            }

            if (doubled <= spanX)
            {
                error += spanX;
                walked.y += stepY;
            }
        }
    } // GCOVR_EXCL_LINE

    std::vector<Pixel> ellipsePixels(
        const Pixel from, const Pixel to, const Size within)
    {
        const auto across = boundsOf(from.x, to.x);
        const auto down = boundsOf(from.y, to.y);

        const auto first = std::max<std::int64_t>(down.low, 0);
        const auto last = std::min<std::int64_t>(
            down.high, static_cast<std::int64_t>(within.height) - 1);

        const auto left = std::max<std::int64_t>(across.low, 0);
        const auto right = std::min<std::int64_t>(
            across.high, static_cast<std::int64_t>(within.width) - 1);

        std::vector<Pixel> covered;

        for (auto y = first; y <= last; ++y)
        {
            for (auto x = left; x <= right; ++x)
            {
                if (ringsEllipse(x, y, across, down))
                {
                    covered.push_back(
                        Pixel{
                            .x = static_cast<std::int32_t>(x),
                            .y = static_cast<std::int32_t>(y)});
                }
            }
        }

        return covered;
    } // GCOVR_EXCL_LINE

    std::vector<Pixel> shapePixels(
        const Tool tool,
        const Pixel from,
        const Pixel to,
        const Size within)
    {
        return tool == Tool::Ellipse ? ellipsePixels(from, to, within)
                                     : linePixels(from, to, within);
    } // GCOVR_EXCL_LINE

}
