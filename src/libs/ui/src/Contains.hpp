#pragma once

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui::detail
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    [[nodiscard]] inline bool contains(
        const Rect &rect, Point point) noexcept
    {
        const auto left = static_cast<std::int64_t>(rect.origin.x);
        const auto top = static_cast<std::int64_t>(rect.origin.y);
        const auto x = static_cast<std::int64_t>(point.x);
        const auto y = static_cast<std::int64_t>(point.y);

        return x >= left && x < left + rect.size.width && y >= top
               && y < top + rect.size.height;
    }

}
