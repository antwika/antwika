#include "antwika/game/WalkerMotion.hpp"

#include <cstdint>

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/tween/Tween.hpp>

#include "antwika/game/SpriteBounds.hpp"
#include "antwika/game/TileAtlas.hpp"
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

    std::uint32_t walkerFrame(
        const WalkerSprite &walker, Progress subTick)
    {
        if (!walker.from.has_value())
        {
            return 0;
        }

        const auto phase = stepPhase(walker.ticksIntoStep, subTick);

        return static_cast<std::uint32_t>(
            phase.numerator() * kWalkCycleFrames / phase.denominator());
    }

    Rect walkerBounds(
        const AtlasSpecs &specs,
        const WalkerSprite &walker,
        const Camera &camera,
        Progress subTick)
    {
        const auto to = blockAnchor(walker.at, Footprint{}, camera);
        const auto from = blockAnchor(
            walker.from.value_or(walker.at), Footprint{}, camera);

        const auto phase = stepPhase(walker.ticksIntoStep, subTick);

        const antwika::gfx::Point anchor{
            .x = static_cast<std::int32_t>(
                antwika::tween::tweenBetween(
                    from.x, to.x, kWalkerEasing, phase)),
            .y = static_cast<std::int32_t>(
                antwika::tween::tweenBetween(
                    from.y, to.y, kWalkerEasing, phase))};

        return spriteBounds(specs.walker, anchor, camera);
    }

}
