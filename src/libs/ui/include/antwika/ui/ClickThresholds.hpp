#pragma once

#include <cstdint>
#include <antwika/gfx/PointF.hpp>

namespace antwika::ui
{

    inline constexpr std::uint32_t kDoubleClickFrames = 24;

    inline constexpr float kDoubleClickRadius = 4.0F;

    struct ClickThresholds final
    {
        std::uint32_t frames = kDoubleClickFrames;

        float radius = kDoubleClickRadius;

        [[nodiscard]] bool operator==(
            const ClickThresholds &other) const
            = default;
    };

}
