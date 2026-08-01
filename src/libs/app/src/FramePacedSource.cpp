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
        FramePacing pacing)
        : inner(inner), pass(pass), sleeper(sleeper), pacing(pacing)
    {
        if (pacing.framesPerTick == 0)
        {
            throw FramePacingError(
                "a tick must be drawn at least once");
        }
    }

    std::vector<Event> FramePacedSource::eventsFor(antwika::time::Tick tick)
    {
        const auto frames =
            static_cast<antwika::time::Tick>(pacing.framesPerTick);

        // Integer division, so the slices are whole milliseconds.
        // The last wait absorbs whatever they left over.
        // So a tick lasts exactly as long however it was cut up.
        const auto slice = pacing.tickInterval / pacing.framesPerTick;
        auto spent = std::chrono::milliseconds{0};

        for (antwika::time::Tick frame = 1; frame < frames; ++frame)
        {
            sleeper.sleep(slice);
            spent += slice;
            pass.draw(antwika::animation::Progress(frame, frames));
        }

        sleeper.sleep(pacing.tickInterval - spent);

        return inner.eventsFor(tick);
    }

} // namespace antwika::app
