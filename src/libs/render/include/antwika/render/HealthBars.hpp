#pragma once

#include <array>
#include <cstddef>

#include <antwika/component/Health.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

namespace antwika::render
{

    inline constexpr float kHealthBarWide = 12.0F;

    inline constexpr float kHealthBarTall = 2.0F;

    inline constexpr float kHealthBarGap = 1.0F;

    inline constexpr float kHealthBarLift = 3.0F;

    inline constexpr std::size_t kHealthBarParts = 4;

    [[nodiscard]] std::array<gfx::RectF, kHealthBarParts> getHealthBars(
        gfx::PointF overheadPoint, component::Health health) noexcept;

}
