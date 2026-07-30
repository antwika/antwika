#pragma once

#include <chrono>

#include "IClock.hpp"

namespace antwika::time
{

    /**
     * @brief IClock implementation backed by std::chrono::system_clock.
     */
    class SystemClock final : public IClock
    {
    public:
        /**
         * @brief Get the current wall-clock time.
         * @return The result of std::chrono::system_clock::now().
         */
        [[nodiscard]] std::chrono::time_point<std::chrono::system_clock>
        now() const noexcept override;
    };
} // namespace antwika::time
