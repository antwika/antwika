#pragma once

#include <cstddef>
#include <string>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct TextEdit final
    {
        WidgetId fieldWidget = kNoWidget;

        std::string text{};

        std::size_t cursor = 0;

        std::size_t anchor = 0;

        std::string copiedText{};

        bool submitted = false;

        bool cancelled = false;

        [[nodiscard]] bool operator==(const TextEdit &other) const =
            default;
    };

}
