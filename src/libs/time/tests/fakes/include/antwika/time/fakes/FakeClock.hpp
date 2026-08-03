#pragma once

#include <antwika/time/IClock.hpp>

namespace antwika::time::fakes
{

    using antwika::time::IClock;

    /**
     * @brief IClock implementation with a manually controlled time value.
     *
     * Intended for tests that need deterministic, reproducible timestamps.
     */
    class FakeClock final : public IClock
    {
    public:
        /**
         * @brief Construct the clock with an initial time value.
         * @param t The time point to report until advanced or set.
         */
        explicit FakeClock(std::chrono::time_point<std::chrono::system_clock> t)
            : current(t)
        {
        }

        /**
         * @brief Get the currently configured time.
         * @return The last value set via the constructor, set(), or advance().
         */
        [[nodiscard]] std::chrono::time_point<std::chrono::system_clock>
        now() const noexcept override
        {
            return current;
        }

        /**
         * @brief Move the clock forward by a duration.
         *
         * Milliseconds rather than seconds, so a test pacing something
         * inside one tick can say what it means; a whole number of
         * seconds still converts implicitly and reads the same.
         *
         * @param by How much to add to the current time.
         */
        void advance(std::chrono::milliseconds by)
        {
            current += by;
        }

        /**
         * @brief Replace the clock's current time value.
         * @param t The new time point to report.
         */
        void set(std::chrono::time_point<std::chrono::system_clock> t)
        {
            current = t;
        }

    private:
        std::chrono::time_point<std::chrono::system_clock> current;
    };
} // namespace antwika::time::fakes
