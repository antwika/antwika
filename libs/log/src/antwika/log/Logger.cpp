#include "antwika/log/Logger.hpp"

#include <iostream>
#include <string>

namespace antwika::log
{

    Logger::Logger(Level level, Appender &appender) : level(level), appender(appender)
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
        std::string formatted;
        formatted.reserve(level.size() + message.size() + 4);
        formatted.append("[");
        formatted.append(level);
        formatted.append("] ");
        formatted.append(message);
        appender.append(formatted);
    }

} // namespace antwika::log
