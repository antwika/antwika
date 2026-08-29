#pragma once

#include <cstdint>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct EdgeChange final
    {
        WidgetId edgeWidget = kNoWidget;

        std::uint32_t extent = 0;

        [[nodiscard]] bool operator==(const EdgeChange &other) const =
            default;
    };

}
