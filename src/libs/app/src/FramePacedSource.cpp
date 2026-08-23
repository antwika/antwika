#include "antwika/app/FramePacedSource.hpp"

#include <chrono>
#include <functional>
#include <optional>
#include <vector>

#include <antwika/animation/Progress.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/input/IFramePump.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/app/FramePacingError.hpp"

namespace antwika::app
{

    using antwika::event::ITickEventSource;

    FramePacedSource::FramePacedSource(
        ITickEventSource &innerSource,
        IFramePass &pass,
        ISleeper &sleeper,
        const antwika::time::IClock &clock,
        FramePacing pacing,
        std::optional<std::reference_wrapper<antwika::input::IFramePump>>
            pump,
        std::optional<std::reference_wrapper<IFramePacingSink>>
            pacingSink)
        : innerSource(innerSource),
          pass(pass),
          sleeper(sleeper),
          clock(clock),
          pacing(pacing),
          pump(pump),
          pacingSink(pacingSink)
    {
        if (pacing.framesPerTick == 0)
        {
            throw FramePacingError(
                "a tick must be drawn at least once");
        }
    }

    bool FramePacedSource::waitUntil(
        std::chrono::time_point<std::chrono::system_clock> startTime,
        std::chrono::microseconds elapsedTime)
    {
        const auto left = startTime + elapsedTime - clock.getCurrentTime();

        if (left < left.zero())
        {
            return false;
        }

        sleeper.sleep(
            std::chrono::duration_cast<std::chrono::milliseconds>(left));

        return true;
    }

    void FramePacedSource::sample(antwika::time::Tick tick)
    {
        if (pump.has_value())
        {
            pump->get().pollFrame(tick);
        }
    }

    std::vector<Event> FramePacedSource::eventsFor(antwika::time::Tick tick)
    {
        const auto frames =
            static_cast<antwika::time::Tick>(pacing.framesPerTick);

        const auto startTime = clock.getCurrentTime();

        const std::chrono::microseconds interval = pacing.tickInterval;

        const auto slice = interval / pacing.framesPerTick;

        for (antwika::time::Tick frame = 1; frame < frames; ++frame)
        {
            if (!waitUntil(startTime, slice * frame))
            {
                if (pacingSink.has_value())
                {
                    pacingSink->get().onFrameDropped(tick);
                }

                continue;
            }

            sample(tick);

            pass.draw(antwika::animation::Progress(frame, frames));

            if (pacingSink.has_value())
            {
                pacingSink->get().onFrameDrawn(tick);
            }
        }

        (void)waitUntil(startTime, interval);

        return innerSource.eventsFor(tick);
    }

}
