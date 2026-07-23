#include "antwika/log/Logger.hpp"

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace antwika::log
{

    Logger::Logger(antwika::time::IClock& clock, Level level, Appender &appender) : clock(clock), level(level), appender(appender)
    {
    }

    void Logger::trace(std::string_view message)
    {
        if (Level::Trace < level)
            return;
        log("TRACE", message);
    }

    void Logger::debug(std::string_view message)
    {
        if (Level::Debug < level)
            return;
        log("DEBUG", message);
    }

    void Logger::info(std::string_view message)
    {
        if (Level::Info < level)
            return;
        log("INFO", message);
    }

    void Logger::warning(std::string_view message)
    {
        if (Level::Warning < level)
            return;
        log("WARNING", message);
    }

    void Logger::error(std::string_view message)
    {
        if (Level::Error < level)
            return;
        log("ERROR", message);
    }

    void Logger::fatal(std::string_view message)
    {
        if (Level::Fatal < level)
            return;
        log("FATAL", message);
    }

    void Logger::log(std::string_view level, std::string_view message)
    {
        auto t = std::chrono::system_clock::to_time_t(clock.now());

        std::tm tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << '[' << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ["
            << level << "] "
            << message;

        appender.append(oss.str());
    }

} // namespace antwika::log
