#pragma once

#include <cstddef>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct OptionChoice final
    {
        WidgetId dropdown = kNoWidget;

        std::size_t index = 0;

        [[nodiscard]] bool operator==(const OptionChoice &other) const =
            default;
    };

}
