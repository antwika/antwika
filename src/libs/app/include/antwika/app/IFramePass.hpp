#pragma once

#include <antwika/animation/Progress.hpp>

namespace antwika::app
{

    /**
     * @brief Draws one frame somewhere between two ticks.
     *
     * The seam that lets a picture be redrawn more often than the
     * simulation advances, so something moving a whole cell per step can
     * be shown part of the way there.
     *
     * **It is handed no World, no Tick, no dispatcher and no event
     * source, and that absence is the whole safety condition.** A pass
     * between two ticks must not change what the simulation computes, and
     * the way that is guaranteed here is that an implementation is given
     * nothing it could change -- rather than being given the world and
     * asked to promise. An implementation therefore has to have been
     * handed whatever it draws from before the frame began, which in
     * practice means a snapshot taken on the tick.
     *
     * The one argument is how far through the tick the frame is, as an
     * exact fraction. It is animation::Progress rather than a float
     * because a picture asserted call by call in a test has to be the
     * same picture on every toolchain, and because the rest of this
     * project's sub-tick arithmetic is already rational.
     */
    class IFramePass
    {
    public:
        virtual ~IFramePass() = default;

        /**
         * @brief Draw one frame.
         * @param subTick How far through the current tick this frame
         * falls, zero on the frame the tick itself draws.
         */
        virtual void draw(antwika::animation::Progress subTick) = 0;
    };

} // namespace antwika::app
