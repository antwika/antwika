#include "antwika/animation/Playback.hpp"

#include <cstddef>

#include <antwika/time/Tick.hpp>

#include "antwika/animation/AnimationError.hpp"
#include "antwika/animation/Clip.hpp"
#include "antwika/animation/DirectionalClipSet.hpp"
#include "antwika/animation/Facing.hpp"
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

        // A clip is never zero ticks long and no frame lasts zero ticks.
        // So this remainder always lands inside a frame.
        // That is what stops the walk below, which has no bound of its own.
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

    Frame resolve(
        const DirectionalClipSet &clips,
        Facing facing,
        time::Tick elapsedTicks)
    {
        return resolve(clips.clipFor(facing), elapsedTicks);
    }

    Progress stepProgress(time::Tick elapsedTicks, time::Tick ticksPerStep)
    {
        if (ticksPerStep == 0)
        {
            throw AnimationError("A step must last at least one tick");
        }

        return Progress(elapsedTicks % ticksPerStep, ticksPerStep);
    }

} // namespace antwika::animation
