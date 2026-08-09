#pragma once

#include <cstdint>

#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    inline constexpr std::uint32_t kWholeSplit = 1000;

    struct SplitSpec final
    {
        WidgetId id = kNoWidget;

        Axis axis = Axis::Row;

        Sizing width = kGrow;

        Sizing height = kGrow;

        /**
         * @brief How much of the axis the first pane takes.
         *
         * Requires: no more than kWholeSplit, which is the whole of it.
         */
        std::uint32_t ratio = kWholeSplit / 2;

        std::uint32_t minimum = 0;

        bool dragging = false;

        [[nodiscard]] bool operator==(const SplitSpec &other) const =
            default;
    };

}
