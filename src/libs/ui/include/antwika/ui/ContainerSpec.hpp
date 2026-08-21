#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Color.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    using antwika::gfx::Color;

    struct ContainerSpec final
    {
        Sizing widthSizing = kGrowSizing;

        Sizing heightSizing = kFitSizing;

        Alignment crossAlignment = Alignment::Start;

        std::optional<Color> backgroundColor{};

        std::optional<std::uint32_t> padding{};

        std::optional<std::uint32_t> gap{};

        WidgetId widgetId = kNoWidget;

        bool clips = false;
    };

}
