#pragma once

#include <string>

namespace antwika::log
{

    /**
     * @brief Severity of a log record, ordered from least to most severe.
     */
    enum class Level
    {
        Trace = 0,
        Debug = 10,
        Info = 20,
        Warning = 30,
        Error = 40,
        Fatal = 50,
    };

    /**
     * @brief Get the human-readable name of a severity level.
     * @param level The level to name.
     * @return The level's upper-case name, e.g. "INFO" for Level::Info.
     */
    [[nodiscard]] std::string_view toString(Level level) noexcept;

} // namespace antwika::log
