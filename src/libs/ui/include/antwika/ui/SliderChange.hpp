#pragma once

#include <cstdint>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct SliderChange final
    {
        WidgetId slider = kNoWidget;

        std::uint32_t value = 0;

        [[nodiscard]] bool operator==(const SliderChange &other) const =
            default;
    };

}
