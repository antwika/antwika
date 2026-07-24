#include "antwika/log/Logger.hpp"

#include <chrono>
#include <format>
#include <string>

namespace antwika::log
{

    Logger::Logger(antwika::time::IClock &clock, Level level, Appender &appender) : clock(clock), level(level), appender(appender)
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
        const auto t = std::chrono::system_clock::to_time_t(clock.now());

        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        appender.append(std::format(
            "[{:04}-{:02}-{:02} {:02}:{:02}:{:02}] [{}] {}",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec,
            level,
            message));
    }

} // namespace antwika::log
