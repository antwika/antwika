#pragma once

#include <chrono>
#include <vector>

#include <antwika/time/ISleeper.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

namespace antwika::app::fakes
{

    using antwika::time::fakes::FakeClock;

    class FakeOvershootingSleeper final : public antwika::time::ISleeper
    {
    public:
        FakeOvershootingSleeper(
            FakeClock &clock, std::chrono::milliseconds overshoot)
            : clock(clock), overshoot(overshoot)
        {
        }

        void sleep(std::chrono::milliseconds duration) override
        {
            durations.push_back(duration);
            clock.advance(duration + overshoot);
        }

        [[nodiscard]] const std::vector<std::chrono::milliseconds>
            &requestedSpans() const noexcept
        {
            return durations;
        }

    private:
        FakeClock &clock;
        std::chrono::milliseconds overshoot;
        std::vector<std::chrono::milliseconds> durations;
    };

}
