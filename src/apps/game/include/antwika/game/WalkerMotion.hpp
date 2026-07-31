#pragma once

#include <cstdint>

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::animation::Progress;
    using antwika::gfx::Rect;

    /**
     * @brief Get how far through its step a walker is, frame and all.
     *
     * There are two clocks in play and this folds them into one exact
     * fraction: the whole ticks of the step already gone, and how far
     * through the current tick this frame falls. Multiplying the two
     * denominators rather than rounding either is what keeps the answer
     * exact, so the same frame of the same tick is the same fraction on
     * every toolchain.
     *
     * It never reaches one. The tick a walker arrives is the tick it
     * starts its next step, at which point what it is stepping out of
     * moves up with it -- so the end of one span is the start of the
     * next rather than a position anything is ever drawn at.
     *
     * @param ticksIntoStep Whole ticks of this step already gone, less
     * than kTicksPerStep.
     * @param subTick How far through the current tick this frame is.
     * @return The combined fraction, at least zero and below one.
     */
    [[nodiscard]] Progress stepPhase(
        std::uint8_t ticksIntoStep, Progress subTick);

    /**
     * @brief Get where to draw a walker this frame.
     *
     * A walker with nowhere it came from is drawn on its own cell, since
     * a span from a cell to itself is the same arithmetic with both ends
     * equal -- so a freshly placed walker needs no case of its own here
     * or in whatever draws it.
     *
     * Both ends are the same size, because a zoom is a property of the
     * camera rather than of where a walker happens to be, so only the
     * origin moves.
     *
     * @param walker The walker to place.
     * @param camera Supplies the zoom and the pan.
     * @param subTick How far through the current tick this frame is.
     * @return The box to blit the walker's tile into.
     */
    [[nodiscard]] Rect walkerBounds(
        const WalkerSprite &walker, const Camera &camera, Progress subTick);

} // namespace antwika::game
