#pragma once

#include <chrono>
#include <vector>

#include <antwika/time/ISleeper.hpp>

#include "antwika/time/fakes/FakeClock.hpp"

namespace antwika::time::fakes
{

    using antwika::time::ISleeper;

    class FakeSleeper final : public ISleeper
    {
    public:
        FakeSleeper() = default;

        explicit FakeSleeper(FakeClock &clock) : clock(&clock)
        {
        }

        void sleep(std::chrono::milliseconds duration) override
        {
            durations.push_back(duration);

            if (clock != nullptr)
            {
                clock->advance(duration);
            }
        }

        [[nodiscard]] const std::vector<std::chrono::milliseconds> &
        requested() const noexcept
        {
            return durations;
        }

        [[nodiscard]] std::chrono::milliseconds total() const
        {
            std::chrono::milliseconds sum{0};
            for (const auto duration : durations)
            {
                sum += duration;
            }
            return sum;
        }

    private:
        FakeClock *clock = nullptr;
        std::vector<std::chrono::milliseconds> durations;
    };

}
