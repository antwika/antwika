#pragma once

#include <cstdint>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct SliderSpec final
    {
        WidgetId id = kNoWidget;

        Sizing width = kGrow;

        std::uint32_t value = 0;

        /**
         * @brief The largest value the slider reports.
         *
         * Requires: greater than zero, or the slider reports nothing.
         */
        std::uint32_t range = 100;

        bool dragging = false;

        [[nodiscard]] bool operator==(const SliderSpec &other) const =
            default;
    };

}
