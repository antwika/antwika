#pragma once

#include "IFormatter.hpp"
#include "Level.hpp"

namespace antwika::log
{

    /**
     * @brief IFormatter that renders a log record as plain, unstyled text.
     */
    class PlainFormatter final : public IFormatter
    {
    public:
        /**
         * @brief Format a log record as plain text.
         * @param time The time the record was logged.
         * @param level The severity level of the record.
         * @param message The raw log message.
         * @return The formatted message.
         */
        [[nodiscard]] std::string format(std::chrono::system_clock::time_point time, Level level, std::string_view message) const override;
    };

} // antwika::log
