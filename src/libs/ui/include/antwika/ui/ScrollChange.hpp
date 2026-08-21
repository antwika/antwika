#pragma once

#include <cstddef>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct ScrollChange final
    {
        WidgetId areaWidget = kNoWidget;

        std::size_t line = 0;

        [[nodiscard]] bool operator==(const ScrollChange &other) const =
            default;
    };

}
