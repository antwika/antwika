#pragma once

#include <cstddef>
#include <cstdint>
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct LineRun final
    {
        std::size_t line = 0;

        std::uint32_t rows = 0;

        WidgetId widgetId = kNoWidget;

        [[nodiscard]] bool operator==(const LineRun &other) const
            = default;
    };

}
