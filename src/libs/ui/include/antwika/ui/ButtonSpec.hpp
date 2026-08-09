#pragma once

#include <optional>

#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct ButtonSpec final
    {
        WidgetId id = kNoWidget;

        Sizing width = kFit;

        std::optional<ButtonState> state{};
    };

}
