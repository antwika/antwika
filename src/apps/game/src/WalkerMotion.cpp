#include "antwika/game/WalkerMotion.hpp"

#include <cstdint>

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/tween/Tween.hpp>

#include "antwika/game/SpriteBounds.hpp"
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
        // The anchor slides and the sprite hangs off it.
        // Both ends share a size, being one zoom's one sprite cell.
        // So tweening the anchors says what tweening the boxes did.
        const auto to = blockAnchor(walker.at, Footprint{}, camera);
        const auto from = blockAnchor(
            walker.from.value_or(walker.at), Footprint{}, camera);

        const auto phase = stepPhase(walker.ticksIntoStep, subTick);

        // Through the tween rather than straight to interpolate().
        // The two are the same arithmetic while the easing is linear.
        // So this changed no pixel, and kWalkerEasing is where it would.
        const antwika::gfx::Point anchor{
            .x = static_cast<std::int32_t>(
                antwika::tween::tweenBetween(
                    from.x, to.x, kWalkerEasing, phase)),
            .y = static_cast<std::int32_t>(
                antwika::tween::tweenBetween(
                    from.y, to.y, kWalkerEasing, phase))};

        return spriteBounds(AtlasKind::OneByOne, anchor, camera);
    }

} // namespace antwika::game
