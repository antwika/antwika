#pragma once

#include <cstddef>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    struct HoverTarget final
    {
        WidgetId widgetId = kNoWidget;

        Rect rect{};

        std::size_t command = 0;

        Color idleColor{};

        Color hoveredColor{};

        bool held = false;

        [[nodiscard]] bool operator==(const HoverTarget &other) const =
            default;
    };

}
