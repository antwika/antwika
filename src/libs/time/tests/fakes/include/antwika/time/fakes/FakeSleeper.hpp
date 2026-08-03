#pragma once

#include <chrono>
#include <vector>

#include <antwika/time/ISleeper.hpp>

#include "antwika/time/fakes/FakeClock.hpp"

namespace antwika::time::fakes
{

    using antwika::time::ISleeper;

    /**
     * @brief ISleeper implementation that records instead of waiting.
     *
     * Lets a test assert how something paced itself without spending the
     * wall-clock time doing it.
     *
     * **It may be given a clock to move, and a test of anything that
     * paces against one has to give it.** A real sleeper's whole effect
     * is that time passes; a fake that records a duration and leaves the
     * clock where it was models a machine on which waiting achieves
     * nothing, so code that measures its next wait from what the clock
     * says will ask for a longer one every time. Paired, this is the
     * perfect sleeper -- exactly the duration asked for, and not a
     * moment more.
     */
    class FakeSleeper final : public ISleeper
    {
    public:
        /** @brief Construct a sleeper that moves no clock. */
        FakeSleeper() = default;

        /**
         * @brief Construct a sleeper that moves a clock as it waits.
         * @param clock Advanced by every duration asked for; must
         * outlive this object.
         */
        explicit FakeSleeper(FakeClock &clock) : clock(&clock)
        {
        }

        /**
         * @brief Record a request to wait, and return at once.
         * @param duration The duration asked for.
         */
        void sleep(std::chrono::milliseconds duration) override
        {
            durations.push_back(duration);

            if (clock != nullptr)
            {
                clock->advance(duration);
            }
        }

        /**
         * @brief Read every duration asked for, in order.
         * @return The requested durations.
         */
        [[nodiscard]] const std::vector<std::chrono::milliseconds> &
        requested() const noexcept
        {
            return durations;
        }

        /**
         * @brief Add up every duration asked for.
         * @return How long a real sleeper would have waited in total.
         */
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

} // namespace antwika::time::fakes
