#pragma once

#include <string>
#include <chrono>

#include "Level.hpp"

namespace antwika::log
{

    /**
     * @brief Renders a log record into its final textual form.
     */
    class IFormatter
    {
    public:
        virtual ~IFormatter() = default;

        /**
         * @brief Format a log record as text.
         * @param time The time the record was logged.
         * @param level The severity level of the record.
         * @param message The raw log message.
         * @return The fully formatted message.
         */
        [[nodiscard]] virtual std::string format(
            std::chrono::system_clock::time_point time,
            Level level,
            std::string_view message) const = 0;
    };

} // namespace antwika::log
