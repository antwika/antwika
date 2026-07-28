#pragma once

#include <string>
#include <chrono>

#include "Level.hpp"

namespace antwika::log
{

    /**
     * @brief Decides whether a log record at a given level should be emitted.
     */
    class ILogPolicy
    {
    public:
        virtual ~ILogPolicy() = default;

        /**
         * @brief Check whether a level passes this policy.
         * @param level The severity level to check.
         * @return true if records at this level should be logged, false otherwise.
         */
        [[nodiscard]] virtual bool accepts(Level level) const noexcept = 0;
    };

} // namespace antwika::log
