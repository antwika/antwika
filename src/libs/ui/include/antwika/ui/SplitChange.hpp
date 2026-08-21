#pragma once

#include <cstdint>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct SplitChange final
    {
        WidgetId dividerWidget = kNoWidget;

        std::uint32_t ratio = 0;

        [[nodiscard]] bool operator==(const SplitChange &other) const =
            default;
    };

}
