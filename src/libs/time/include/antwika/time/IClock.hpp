#pragma once

#include <chrono>

namespace antwika::time
{

    /**
     * @brief Abstract source of the current time.
     *
     * Allows callers to depend on time without coupling to a specific
     * clock implementation, so that a fake or fixed clock can be
     * substituted in tests.
     */
    class IClock
    {
    public:
        virtual ~IClock() = default; // GCOVR_EXCL_LINE

        /**
         * @brief Get the current point in time.
         * @return The current time as a system_clock time point.
         */
        [[nodiscard]] virtual std::chrono::time_point<std::chrono::system_clock> now() const noexcept = 0;
    };
} // namespace antwika::time
