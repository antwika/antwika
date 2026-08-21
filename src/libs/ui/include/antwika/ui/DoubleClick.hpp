#pragma once

#include <cstdint>

#include <antwika/gfx/PointF.hpp>

#include "antwika/ui/ClickThresholds.hpp"
#include "antwika/ui/ClickTrack.hpp"

namespace antwika::ui
{

    [[nodiscard]] bool isDoubleClick(
        const ClickTrack &track,
        std::uint32_t nowTick,
        gfx::PointF point,
        ClickThresholds withinThresholds = {});

    [[nodiscard]] ClickTrack trackClick(
        std::uint32_t nowTick, gfx::PointF point);

}
