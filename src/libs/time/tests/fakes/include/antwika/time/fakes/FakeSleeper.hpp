#pragma once

#include <chrono>
#include <vector>

#include <antwika/time/ISleeper.hpp>

namespace antwika::time::fakes
{

    using antwika::time::ISleeper;

    /**
     * @brief ISleeper implementation that records instead of waiting.
     *
     * Lets a test assert how something paced itself without spending the
     * wall-clock time doing it.
     */
    class FakeSleeper final : public ISleeper
    {
    public:
        /**
         * @brief Record a request to wait, and return at once.
         * @param duration The duration asked for.
         */
        void sleep(std::chrono::milliseconds duration) override
        {
            durations.push_back(duration);
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
        std::vector<std::chrono::milliseconds> durations;
    };

} // namespace antwika::time::fakes
