#include "antwika/ui/Overlays.hpp"

#include "Contains.hpp"

namespace antwika::ui
{

    bool overlaid(
        const Overlays &overlays, HoverPointer hover) noexcept
    {
        if (!hover.position)
        {
            return false;
        }

        for (const Rect &rect : overlays)
        {
            if (detail::contains(rect, *hover.position))
            {
                return true;
            }
        }

        return false;
    }

}
