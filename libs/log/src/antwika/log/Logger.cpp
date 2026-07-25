#include "antwika/log/Logger.hpp"

#include <chrono>
#include <format>
#include <string>

namespace antwika::log
{

    Logger::Logger(IFormatter& formatter, antwika::time::IClock &clock, Level level, IAppender &appender) : formatter(formatter), clock(clock), level(level), appender(appender)
    {
    }

    void Logger::trace(std::string_view message) noexcept
    {
        if (Level::Trace < level)
            return;
        log("TRACE", message);
    }

    void Logger::debug(std::string_view message) noexcept
    {
        if (Level::Debug < level)
            return;
        log("DEBUG", message);
    }

    void Logger::info(std::string_view message) noexcept
    {
        if (Level::Info < level)
            return;
        log("INFO", message);
    }

    void Logger::warning(std::string_view message) noexcept
    {
        if (Level::Warning < level)
            return;
        log("WARNING", message);
    }

    void Logger::error(std::string_view message) noexcept
    {
        if (Level::Error < level)
            return;
        log("ERROR", message);
    }

    void Logger::fatal(std::string_view message) noexcept
    {
        log("FATAL", message);
    }

    void Logger::log(std::string_view level, std::string_view message) noexcept
    {
        const auto now = clock.now();
        try {
            auto formatted = formatter.format(now, level, message);
            appender.append(formatted);
        } catch (...) {
            // Ignore, perhaps add a fallback
        }
    }

} // namespace antwika::log
