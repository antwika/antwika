#pragma once

#include <cstddef>
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct TextHighlight final
    {
        std::size_t begin = 0;

        std::size_t end = 0;

        [[nodiscard]] bool operator==(const TextHighlight &other) const
            = default;
    };

}
