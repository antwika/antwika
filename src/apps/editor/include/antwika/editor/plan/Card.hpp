#pragma once

#include <string>
#include <antwika/gfx/Rect.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::editor
{

    struct Card final
    {
        std::string title;

        std::string body;

        [[nodiscard]] bool operator==(const Card &other) const
            = default;
    };

}
