#pragma once

#include <cstddef>
#include <cstdint>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    struct Splitter final
    {
        WidgetId widgetId = kNoWidget;

        std::size_t split = 0;

        std::size_t divider = 0;

        Axis axis = Axis::Row;

        std::uint32_t minimum = 0;

        std::uint32_t ratio = 0;

        bool dragging = false;
    };

}
