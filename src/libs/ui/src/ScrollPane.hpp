#pragma once

#include <cstddef>
#include <cstdint>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    struct ScrollPane final
    {
        WidgetId widgetId = kNoWidget;

        std::size_t viewport = 0;

        std::size_t track = 0;

        std::size_t thumb = 0;

        std::size_t requestedOffset = 0;

        std::uint32_t step = 1;

        bool dragging = false;
    };

}
