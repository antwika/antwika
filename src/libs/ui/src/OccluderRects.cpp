#include "antwika/ui/OccluderRects.hpp"

#include <algorithm>
#include <cstdint>

#include "Contains.hpp"

namespace antwika::ui
{

    namespace
    {
        [[nodiscard]] bool meets(
            const Rect &oneRect, const Rect &otherRect) noexcept
        {
            const auto left = static_cast<std::int64_t>(oneRect.originPoint.x);
            const auto top = static_cast<std::int64_t>(oneRect.originPoint.y);
            const auto otherLeft =
                static_cast<std::int64_t>(otherRect.originPoint.x);
            const auto otherTop =
                static_cast<std::int64_t>(otherRect.originPoint.y);

            const auto inLeft = std::max(left, otherLeft);
            const auto inRight = std::min(
                left + oneRect.size.width,
                otherLeft + otherRect.size.width);
            const auto inTop = std::max(top, otherTop);
            const auto inBottom = std::min(
                top + oneRect.size.height,
                otherTop + otherRect.size.height);

            return inLeft < inRight && inTop < inBottom;
        }
    }

    bool isOccluded(
        const OccluderRects &occluders, HoverPointer hover) noexcept
    {
        if (!hover.positionPoint)
        {
            return false;
        }

        for (const Rect &rect : occluders)
        {
            if (detail::contains(rect, *hover.positionPoint))
            {
                return true;
            }
        }

        return false;
    }

    bool isOccluded(
        const OccluderRects &occluders, const Rect &rect) noexcept
    {
        for (const Rect &occluder : occluders)
        {
            if (meets(occluder, rect))
            {
                return true;
            }
        }

        return false;
    }

}
