#include "antwika/render/HealthBars.hpp"

#include <cmath>
#include <cstdint>

#include <antwika/component/Health.hpp>
#include <antwika/gfx/SizeF.hpp>

namespace antwika::render
{

    namespace
    {
        [[nodiscard]] gfx::RectF pixels(
            const float left,
            const float top,
            const float width,
            const float height) noexcept
        {
            return gfx::RectF(
                gfx::PointF{std::round(left), std::round(top)},
                gfx::SizeF{std::round(width), height});
        }

        [[nodiscard]] float filledWidth(
            const std::uint16_t level) noexcept
        {
            return kHealthBarWide * static_cast<float>(level)
                   / static_cast<float>(component::kFullHealth);
        }
    }

    std::array<gfx::RectF, kHealthBarParts> healthBars(
        const gfx::PointF overheadPoint,
        const component::Health health) noexcept
    {
        const auto left = overheadPoint.x - (kHealthBarWide / 2.0F);
        const auto waterTop =
            overheadPoint.y - kHealthBarLift - kHealthBarTall;
        const auto foodTop = waterTop - kHealthBarGap - kHealthBarTall;

        return {
            pixels(left, foodTop, kHealthBarWide, kHealthBarTall),
            pixels(
                left,
                foodTop,
                filledWidth(health.food),
                kHealthBarTall),
            pixels(left, waterTop, kHealthBarWide, kHealthBarTall),
            pixels(
                left,
                waterTop,
                filledWidth(health.water),
                kHealthBarTall)};
    } // GCOVR_EXCL_LINE

}
