#include "antwika/ui/WidgetRects.hpp"

#include <algorithm>
#include <optional>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    std::optional<Rect> WidgetRects::find(WidgetId id) const
    {
        const auto at = std::ranges::find(entries, id, &WidgetRect::id);

        if (at == entries.end())
        {
            return {};
        }

        return at->rect;
    }

}
