#pragma once

#include <cstddef>
#include <cstdint>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    struct Rail final
    {
        WidgetId id = kNoWidget;

        std::size_t track = 0;

        std::size_t thumb = 0;

        std::uint32_t value = 0;

        std::uint32_t range = 0;

        bool dragging = false;
    };

}
