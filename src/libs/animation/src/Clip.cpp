#include "antwika/animation/Clip.hpp"

#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/animation/AnimationError.hpp"
#include "antwika/animation/KeyFrame.hpp"
#include "antwika/animation/LoopMode.hpp"

namespace antwika::animation
{

    namespace
    {

        time::Tick totalDurationOf(const std::vector<KeyFrame> &keyFrames)
        {
            constexpr time::Tick kMaxTick =
                std::numeric_limits<time::Tick>::max();

            if (keyFrames.empty())
            {
                throw AnimationError("Clip must have at least one frame");
            }

            time::Tick totalTick = 0;

            for (std::size_t i = 0; i < keyFrames.size(); ++i)
            {
                const time::Tick frameDurationTick = keyFrames[i].durationTicks;

                if (frameDurationTick == 0)
                {
                    throw AnimationError(
                        "Clip frame " + std::to_string(i)
                        + " lasts zero ticks");
                }

                if (totalTick > kMaxTick - frameDurationTick)
                {
                    throw AnimationError(
                        "Clip total duration overflows a tick at frame "
                        + std::to_string(i));
                }

                totalTick += frameDurationTick;
            }

            return totalTick;
        }

    }

    Clip::Clip(std::vector<KeyFrame> keyFrames, LoopMode loop)
        : keyFrames(std::move(keyFrames)),
          loopMode(loop),
          durationTick(totalDurationOf(this->keyFrames))
    {
    }

    const std::vector<KeyFrame> &Clip::getFrames() const noexcept
    {
        return keyFrames;
    }

    LoopMode Clip::getLoop() const noexcept
    {
        return loopMode;
    }

    time::Tick Clip::getDurationTicks() const noexcept
    {
        return durationTick;
    }

    Clip getUniformClip(
        std::size_t firstIndex,
        std::size_t frameCount,
        time::Tick ticksPerFrame,
        LoopMode loop)
    {
        std::vector<KeyFrame> keyFrames;
        keyFrames.reserve(frameCount);

        for (std::size_t offset = 0; offset < frameCount; ++offset)
        {
            keyFrames.push_back(KeyFrame{
                .index = firstIndex + offset,
                .durationTicks = ticksPerFrame,
            });
        }

        return Clip(std::move(keyFrames), loop);
    }

}
