#pragma once

#include <optional>
#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/WidgetId.hpp"
#include "antwika/ui/WidgetRect.hpp"

namespace antwika::ui
{

    using antwika::gfx::Rect;

    struct WidgetRects final
    {
        std::vector<WidgetRect> widgetRects{};

        [[nodiscard]] std::optional<Rect> getWidgetRect(WidgetId widget) const;

        [[nodiscard]] bool operator==(const WidgetRects &other) const =
            default;
    };

}
