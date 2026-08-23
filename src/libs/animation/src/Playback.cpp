#include "antwika/animation/Playback.hpp"

#include <cstddef>

#include <antwika/time/Tick.hpp>

#include "antwika/animation/AnimationError.hpp"
#include "antwika/animation/Clip.hpp"
#include "antwika/animation/Frame.hpp"
#include "antwika/animation/KeyFrame.hpp"
#include "antwika/animation/LoopMode.hpp"
#include "antwika/animation/Progress.hpp"

namespace antwika::animation
{

    Frame getFrameAt(const Clip &clip, time::Tick elapsedTicks)
    {
        const time::Tick durationTick = clip.getDurationTicks();

        if (clip.getLoop() == LoopMode::Once && elapsedTicks >= durationTick)
        {
            const KeyFrame &lastFrame = clip.getFrames().back();

            return Frame{
                .index = lastFrame.index,
                .progress =
                    Progress(lastFrame.durationTicks, lastFrame.durationTicks),
                .finished = true,
            };
        }

        time::Tick remainingTick = elapsedTicks % durationTick;
        std::size_t index = 0;

        while (remainingTick >= clip.getFrames()[index].durationTicks)
        {
            remainingTick -= clip.getFrames()[index].durationTicks;
            ++index;
        }

        const KeyFrame &currentFrame = clip.getFrames()[index];

        return Frame{
            .index = currentFrame.index,
            .progress = Progress(remainingTick, currentFrame.durationTicks),
            .finished = false,
        };
    }

    Progress getStepProgress(time::Tick elapsedTicks, time::Tick ticksPerStep)
    {
        if (ticksPerStep == 0)
        {
            throw AnimationError("A step must last at least one tick");
        }

        return Progress(elapsedTicks % ticksPerStep, ticksPerStep);
    }

}
