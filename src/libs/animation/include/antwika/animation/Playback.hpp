#pragma once

#include <antwika/time/Tick.hpp>

#include "antwika/animation/Clip.hpp"
#include "antwika/animation/Frame.hpp"
#include "antwika/animation/Progress.hpp"

namespace antwika::animation
{

    /**
     * @brief Work out which frame a clip is showing.
     *
     * A pure function of its two arguments, which is the whole point of
     * this library: the same tick always gives the same frame, so a
     * replay draws the picture the recorded run drew without the
     * recording having to hold a single frame number.
     *
     * @param clip The clip to read.
     * @param elapsedTicks How many ticks the clip has been running,
     * counted by the caller from whenever it started.
     * @return The frame to show.
     */
    [[nodiscard]] Frame resolve(const Clip &clip, time::Tick elapsedTicks);

    /**
     * @brief Work out how far a movement has got between where it
     * started and where it ends.
     *
     * Pair this with interpolate() to place something part of the way
     * along a step it has not finished taking: the walker in apps/game
     * moves a whole cell per tick, so a caller that wants to draw it
     * between two cells passes elapsed and the step length in a unit
     * finer than a tick and divides the answer itself.
     * This function does not invent that unit, because inventing it
     * would be this library keeping time.
     *
     * @param elapsedTicks How long the movement has been going.
     * @param ticksPerStep How long one whole step takes.
     * @return How far through the current step the elapsed time falls,
     * always below one.
     * @throws AnimationError If ticksPerStep is zero.
     */
    [[nodiscard]] Progress stepProgress(
        time::Tick elapsedTicks, time::Tick ticksPerStep);

} // namespace antwika::animation
