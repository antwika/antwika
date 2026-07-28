#pragma once

#include <string_view>

#include "ILogPolicy.hpp"
#include "Level.hpp"

namespace antwika::log
{

    /**
     * @brief Entry point for emitting log records.
     */
    class ILogger
    {
    public:
        virtual ~ILogger() = default;

        /**
         * @brief Log a message at the given severity level.
         * @param level The severity level of the message.
         * @param message The message to log.
         */
        virtual void log(Level level, std::string_view message) noexcept = 0;
    };

} // namespace antwika::log
