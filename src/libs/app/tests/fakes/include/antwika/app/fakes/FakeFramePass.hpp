#pragma once

#include <cstddef>
#include <vector>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/IFramePass.hpp>

namespace antwika::app::fakes
{

    using antwika::animation::Progress;
    using antwika::app::IFramePass;

    /**
     * @brief IFramePass implementation that records instead of drawing.
     *
     * Lets a test assert which frames were asked for, and how far through
     * a tick each of them fell, without a window or a renderer.
     */
    class FakeFramePass final : public IFramePass
    {
    public:
        /**
         * @brief Record a frame, and draw nothing.
         * @param subTick How far through the tick the frame fell.
         */
        void draw(Progress subTick) override
        {
            frames.push_back(subTick);
        }

        /**
         * @brief Read every frame asked for, in order.
         * @return The sub-tick fractions, one per frame.
         */
        [[nodiscard]] const std::vector<Progress> &drawn() const noexcept
        {
            return frames;
        }

        /**
         * @brief Count how many frames were asked for.
         * @return The number of draw() calls so far.
         */
        [[nodiscard]] std::size_t count() const noexcept
        {
            return frames.size();
        }

    private:
        std::vector<Progress> frames;
    };

} // namespace antwika::app::fakes
