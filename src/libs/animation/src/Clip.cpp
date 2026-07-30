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

            time::Tick total = 0;

            for (std::size_t i = 0; i < keyFrames.size(); ++i)
            {
                const time::Tick frameDuration = keyFrames[i].durationTicks;

                if (frameDuration == 0)
                {
                    throw AnimationError(
                        "Clip frame " + std::to_string(i)
                        + " lasts zero ticks");
                }

                if (total > kMaxTick - frameDuration)
                {
                    throw AnimationError(
                        "Clip total duration overflows a tick at frame "
                        + std::to_string(i));
                }

                total += frameDuration;
            }

            return total;
        }

    } // namespace

    Clip::Clip(std::vector<KeyFrame> keyFrames, LoopMode loop)
        : keyFrames(std::move(keyFrames)),
          loopMode(loop),
          duration(totalDurationOf(this->keyFrames))
    {
    }

    const std::vector<KeyFrame> &Clip::frames() const noexcept
    {
        return keyFrames;
    }

    LoopMode Clip::loop() const noexcept
    {
        return loopMode;
    }

    time::Tick Clip::durationTicks() const noexcept
    {
        return duration;
    }

    Clip uniformClip(
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

} // namespace antwika::animation
