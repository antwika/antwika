#pragma once

#include <cstdint>

#include <antwika/ui/WidgetId.hpp>

namespace antwika::ui
{

    inline constexpr std::uint32_t kTooltipDelayFrames = 60;

    struct HoverTrack final
    {
        WidgetId widget = kNoWidget;

        std::uint32_t sinceFrame = 0;

        [[nodiscard]] bool operator==(const HoverTrack &other) const
            = default;
    };

    [[nodiscard]] HoverTrack updateHover(
        HoverTrack track, WidgetId nowWidget, std::uint32_t clock);

    [[nodiscard]] bool tooltipDue(
        HoverTrack track,
        std::uint32_t clock,
        std::uint32_t delayFrames = kTooltipDelayFrames);

}
