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
        Sizing width = kGrow;

        Sizing height = kFit;

        Alignment cross = Alignment::Start;

        std::optional<Color> background{};

        std::optional<std::uint32_t> padding{};

        std::optional<std::uint32_t> gap{};

        WidgetId id = kNoWidget;
    };

}
