#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Color.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    struct ButtonSpec final
    {
        WidgetId widgetId = kNoWidget;

        Sizing widthSizing = kFitSizing;

        std::optional<ButtonState> state{};

        std::optional<antwika::gfx::Color> fillColor{};

        std::optional<std::uint32_t> wrapWidth{};

        Alignment labelAlignment = Alignment::Center;
    };

}
