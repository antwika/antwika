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
            FakeClock &clock, std::chrono::milliseconds by)
            : clock(clock), by(by)
        {
        }

        void sleep(std::chrono::milliseconds duration) override
        {
            durations.push_back(duration);
            clock.advance(duration + by);
        }

        [[nodiscard]] const std::vector<std::chrono::milliseconds>
            &requested() const noexcept
        {
            return durations;
        }

    private:
        FakeClock &clock;
        std::chrono::milliseconds by;
        std::vector<std::chrono::milliseconds> durations;
    };

}
