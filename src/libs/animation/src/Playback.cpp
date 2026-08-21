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

    Frame resolve(const Clip &clip, time::Tick elapsedTicks)
    {
        const time::Tick durationTick = clip.durationTicks();

        if (clip.loop() == LoopMode::Once && elapsedTicks >= durationTick)
        {
            const KeyFrame &lastFrame = clip.frames().back();

            return Frame{
                .index = lastFrame.index,
                .progress =
                    Progress(lastFrame.durationTicks, lastFrame.durationTicks),
                .finished = true,
            };
        }

        time::Tick remainingTick = elapsedTicks % durationTick;
        std::size_t index = 0;

        while (remainingTick >= clip.frames()[index].durationTicks)
        {
            remainingTick -= clip.frames()[index].durationTicks;
            ++index;
        }

        const KeyFrame &currentFrame = clip.frames()[index];

        return Frame{
            .index = currentFrame.index,
            .progress = Progress(remainingTick, currentFrame.durationTicks),
            .finished = false,
        };
    }

    Progress stepProgress(time::Tick elapsedTicks, time::Tick ticksPerStep)
    {
        if (ticksPerStep == 0)
        {
            throw AnimationError("A step must last at least one tick");
        }

        return Progress(elapsedTicks % ticksPerStep, ticksPerStep);
    }

}
