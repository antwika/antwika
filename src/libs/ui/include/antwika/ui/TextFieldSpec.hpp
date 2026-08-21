#pragma once

#include <cstddef>
#include <limits>
#include <string_view>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    inline constexpr std::size_t kCaretAtEnd =
        std::numeric_limits<std::size_t>::max();

    struct TextFieldSpec final
    {
        WidgetId widgetId = kNoWidget;

        Sizing widthSizing = kGrowSizing;

        std::string_view text{};

        std::string_view placeholder{};

        std::size_t cursor = kCaretAtEnd;

        bool focused = false;
    };

}
