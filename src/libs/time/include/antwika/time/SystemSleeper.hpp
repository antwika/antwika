#pragma once

#include <chrono>

#include "antwika/time/ISleeper.hpp"

namespace antwika::time
{

    /**
     * @brief Sleeper that really waits, on the calling thread.
     */
    class SystemSleeper final : public ISleeper
    {
    public:
        /**
         * @brief Wait for a duration.
         * @param duration How long to wait; a zero or negative duration
         * returns immediately.
         */
        void sleep(std::chrono::milliseconds duration) override;
    };

} // namespace antwika::time
