#include "antwika/ui/HoverHint.hpp"

namespace antwika::ui
{

    HoverTrack getUpdateHover(
        const HoverTrack track,
        const WidgetId nowWidget,
        const std::uint32_t clock)
    {
        if (track.widget == nowWidget)
        {
            return track;
        }

        return HoverTrack{.widget = nowWidget, .sinceFrame = clock};
    }

    bool isTooltipDue(
        const HoverTrack track,
        const std::uint32_t clock,
        const std::uint32_t delayFrames)
    {
        return track.widget != kNoWidget
               && clock >= track.sinceFrame + delayFrames;
    }

}
