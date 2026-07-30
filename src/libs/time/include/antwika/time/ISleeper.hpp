#pragma once

#include <chrono>

namespace antwika::time
{

    /**
     * @brief Abstract way to give up the current thread for a while.
     *
     * Exists so that code which paces itself against wall-clock time --
     * a window drawing one frame per tick, say -- can be tested without
     * the test having to wait. A fake records what it was asked to sleep
     * for and returns at once.
     */
    class ISleeper
    {
    public:
        virtual ~ISleeper() = default;

        /**
         * @brief Stop doing anything for a while.
         * @param duration How long to wait; a zero or negative duration
         * returns immediately.
         */
        virtual void sleep(std::chrono::milliseconds duration) = 0;
    };

} // namespace antwika::time
