#include "antwika/ui/WidgetRects.hpp"

#include <algorithm>
#include <optional>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    std::optional<Rect> WidgetRects::getFind(WidgetId widget) const
    {
        const auto foundRect = std::ranges::find(
            widgetRects,
            widget,
            &WidgetRect::widgetId);

        if (foundRect == widgetRects.end())
        {
            return {};
        }

        return foundRect->rect;
    }

}
