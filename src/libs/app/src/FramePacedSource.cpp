#include "antwika/app/FramePacedSource.hpp"

#include <chrono>
#include <vector>

#include <antwika/animation/Progress.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/app/FramePacingError.hpp"

namespace antwika::app
{

    FramePacedSource::FramePacedSource(
        ITickEventSource &inner,
        IFramePass &pass,
        ISleeper &sleeper,
        const antwika::time::IClock &clock,
        FramePacing pacing)
        : inner(inner),
          pass(pass),
          sleeper(sleeper),
          clock(clock),
          pacing(pacing)
    {
        if (pacing.framesPerTick == 0)
        {
            throw FramePacingError(
                "a tick must be drawn at least once");
        }
    }

    bool FramePacedSource::waitUntil(
        std::chrono::time_point<std::chrono::system_clock> started,
        std::chrono::milliseconds elapsed)
    {
        const auto left = started + elapsed - clock.now();

        // Gone by, so there is nothing left to wait for.
        // Answered rather than slept, since the caller draws nothing.
        // A frame drawn late is what makes a tick outlast its interval.
        // And the tick's own pace is the thing being kept.
        // A moment that is exactly now has not gone by.
        // So the frame due at it is drawn with no wait at all.
        // Which is also what makes an unpaced run draw its frames.
        if (left < left.zero())
        {
            return false;
        }

        // Truncated rather than rounded up.
        // So a wait never overshoots the point it is for.
        // Under a millisecond left is no wait at all.
        // The frame is then drawn straight away.
        // Which is what lets a rate rise past one a millisecond.
        sleeper.sleep(
            std::chrono::duration_cast<std::chrono::milliseconds>(left));

        return true;
    }

    std::vector<Event> FramePacedSource::eventsFor(antwika::time::Tick tick)
    {
        const auto frames =
            static_cast<antwika::time::Tick>(pacing.framesPerTick);

        // Read once, and every wait below is measured from it.
        // So an overshoot is absorbed by the next wait.
        // Rather than added to the tick -- see the class comment.
        const auto started = clock.now();

        // Integer division, so a due time is a whole millisecond.
        // The last wait is against the interval itself.
        // Rather than against a multiple of this.
        // So a tick lasts exactly as long however it was cut up.
        const auto slice = pacing.tickInterval / pacing.framesPerTick;

        for (antwika::time::Tick frame = 1; frame < frames; ++frame)
        {
            if (waitUntil(started, slice * frame))
            {
                pass.draw(antwika::animation::Progress(frame, frames));
            }
        }

        (void)waitUntil(started, pacing.tickInterval);

        return inner.eventsFor(tick);
    }

} // namespace antwika::app
