#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    inline constexpr std::size_t kNoOption =
        std::numeric_limits<std::size_t>::max();

    enum class OptionMark : std::uint8_t
    {
        None,
        Off,
        On,
    };

    struct DropdownSpec final
    {
        WidgetId widgetId = kNoWidget;

        WidgetId optionIdBaseWidget = kNoWidget;

        Sizing widthSizing = kFitSizing;

        std::span<const std::string_view> options{};

        std::size_t selectedIndex = kNoOption;

        std::span<const OptionMark> markedOptions{};

        std::string_view placeholder{};

        bool open = false;
    };

}
