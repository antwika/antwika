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
        const time::Tick duration = clip.durationTicks();

        if (clip.loop() == LoopMode::Once && elapsedTicks >= duration)
        {
            const KeyFrame &last = clip.frames().back();

            return Frame{
                .index = last.index,
                .progress =
                    Progress(last.durationTicks, last.durationTicks),
                .finished = true,
            };
        }

        time::Tick remaining = elapsedTicks % duration;
        std::size_t index = 0;

        while (remaining >= clip.frames()[index].durationTicks)
        {
            remaining -= clip.frames()[index].durationTicks;
            ++index;
        }

        const KeyFrame &current = clip.frames()[index];

        return Frame{
            .index = current.index,
            .progress = Progress(remaining, current.durationTicks),
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
