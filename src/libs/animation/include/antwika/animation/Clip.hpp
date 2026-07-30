#pragma once

#include <cstddef>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/animation/KeyFrame.hpp"
#include "antwika/animation/LoopMode.hpp"

namespace antwika::animation
{

    /**
     * @brief An ordered run of frames and how long each of them stays
     * up, plus what happens at the end.
     *
     * A clip is a definition, not a player: it holds no elapsed time, no
     * current frame and no clock.
     * Which frame it is showing is a question you ask with a tick, and
     * the answer is resolve() in Playback.hpp.
     *
     * Every way of building an invalid clip is refused by the
     * constructor, so a Clip that exists can always be resolved, and
     * resolve() has nothing to check and no failure to report.
     */
    class Clip final
    {
    public:
        /**
         * @brief Build a clip from its frames.
         * @param keyFrames The frames, in the order they are shown.
         * @param loop What to do once the last frame has been shown.
         * @throws AnimationError If there are no frames, if any frame
         * lasts zero ticks, or if the total duration would not fit in a
         * antwika::time::Tick.
         */
        explicit Clip(
            std::vector<KeyFrame> keyFrames,
            LoopMode loop = LoopMode::Loop);

        /**
         * @brief Get the frames, in the order they are shown.
         * @return The frames, never empty.
         */
        [[nodiscard]] const std::vector<KeyFrame> &frames() const noexcept;

        /**
         * @brief Get what happens once the last frame has been shown.
         * @return The loop mode.
         */
        [[nodiscard]] LoopMode loop() const noexcept;

        /**
         * @brief Get how long one pass through every frame takes.
         * @return The sum of the frame durations, never zero.
         */
        [[nodiscard]] time::Tick durationTicks() const noexcept;

    private:
        std::vector<KeyFrame> keyFrames;
        LoopMode loopMode;
        time::Tick duration;
    };

    /**
     * @brief Build a clip whose frames are consecutive indices of equal
     * length.
     *
     * This is the shape a texture atlas row already has, and writing it
     * out frame by frame would be the same three numbers repeated.
     *
     * @param firstIndex The index of the first frame.
     * @param frameCount How many consecutive indices to use.
     * @param ticksPerFrame How long each frame stays up.
     * @param loop What to do once the last frame has been shown.
     * @return The clip.
     * @throws AnimationError If frameCount or ticksPerFrame is zero, or
     * if the total duration would not fit in a antwika::time::Tick.
     */
    [[nodiscard]] Clip uniformClip(
        std::size_t firstIndex,
        std::size_t frameCount,
        time::Tick ticksPerFrame,
        LoopMode loop = LoopMode::Loop);

} // namespace antwika::animation
