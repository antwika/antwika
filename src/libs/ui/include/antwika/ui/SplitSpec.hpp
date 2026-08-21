#pragma once

#include <cstdint>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    inline constexpr std::uint32_t kSplitRatioScale = 1000;

    struct SplitSpec final
    {
        WidgetId widgetId = kNoWidget;

        Axis axis = Axis::Row;

        Sizing widthSizing = kGrowSizing;

        Sizing heightSizing = kGrowSizing;

        std::uint32_t ratio = kSplitRatioScale / 2;

        std::uint32_t minimum = 0;

        bool dragging = false;

        [[nodiscard]] bool operator==(const SplitSpec &other) const =
            default;
    };

}
