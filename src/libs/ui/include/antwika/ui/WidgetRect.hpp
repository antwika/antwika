#pragma once

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    using antwika::gfx::Rect;

    struct WidgetRect final
    {
        WidgetId widgetId = kNoWidget;

        Rect rect{};

        [[nodiscard]] bool operator==(const WidgetRect &other) const =
            default;
    };

}
