#include "antwika/ui/DoubleClick.hpp"

#include <cmath>

namespace antwika::ui
{

    bool isDoubleClick(
        const ClickTrack &track,
        const std::uint32_t nowTick,
        const gfx::PointF point,
        const ClickThresholds withinThresholds)
    {
        if (!track.hasClick || nowTick < track.lastFrame
            || nowTick - track.lastFrame > withinThresholds.frames)
        {
            return false;
        }

        return std::abs(point.x - track.lastPoint.x) <= withinThresholds.radius
               && std::abs(point.y - track.lastPoint.y)
                      <= withinThresholds.radius;
    }

    ClickTrack getTrackClick(
        const std::uint32_t nowTick, const gfx::PointF point)
    {
        return ClickTrack{
            .lastFrame = nowTick, .lastPoint = point, .hasClick = true};
    }

}
