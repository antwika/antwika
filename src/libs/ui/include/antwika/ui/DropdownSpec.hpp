#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <string_view>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    inline constexpr std::size_t kNoOption =
        std::numeric_limits<std::size_t>::max();

    struct DropdownSpec final
    {
        WidgetId id = kNoWidget;

        WidgetId optionIdBase = kNoWidget;

        Sizing width = kFit;

        std::span<const std::string_view> options{};

        std::size_t selected = kNoOption;

        std::string_view placeholder{};

        bool open = false;
    };

}
