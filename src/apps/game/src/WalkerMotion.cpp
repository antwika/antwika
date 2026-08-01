#include "antwika/game/WalkerMotion.hpp"

#include <cstdint>

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/tween/Tween.hpp>

#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    Progress stepPhase(std::uint8_t ticksIntoStep, Progress subTick)
    {
        const auto frames = subTick.denominator();

        return Progress(
            static_cast<antwika::time::Tick>(ticksIntoStep) * frames
                + subTick.numerator(),
            static_cast<antwika::time::Tick>(kTicksPerStep) * frames);
    }

    Rect walkerBounds(
        const WalkerSprite &walker, const Camera &camera, Progress subTick)
    {
        const auto to = cellBounds(walker.at, camera);
        const auto from =
            cellBounds(walker.from.value_or(walker.at), camera);

        const auto phase = stepPhase(walker.ticksIntoStep, subTick);

        // Through the tween rather than straight to interpolate().
        // The two are the same arithmetic while the easing is linear.
        // So this changed no pixel, and kWalkerEasing is where it would.
        return Rect{
            .origin =
                {.x = static_cast<std::int32_t>(
                     antwika::tween::tweenBetween(
                         from.origin.x, to.origin.x, kWalkerEasing, phase)),
                 .y = static_cast<std::int32_t>(
                     antwika::tween::tweenBetween(
                         from.origin.y, to.origin.y, kWalkerEasing, phase))},
            .size = to.size};
    }

} // namespace antwika::game
