#pragma once

#include <cstdint>

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/tween/Easing.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/TileAtlas.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::animation::Progress;
    using antwika::gfx::Rect;

    inline constexpr antwika::tween::Easing kWalkerEasing =
        antwika::tween::Easing::Linear;

    [[nodiscard]] Progress stepPhase(
        std::uint8_t ticksIntoStep, Progress subTick);

    [[nodiscard]] std::uint32_t walkerFrame(
        const WalkerSprite &walker, Progress subTick);

    [[nodiscard]] Rect walkerBounds(
        const AtlasSpecs &specs,
        const WalkerSprite &walker,
        const Camera &camera,
        Progress subTick);

}
