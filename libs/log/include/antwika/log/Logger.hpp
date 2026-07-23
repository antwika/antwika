#pragma once

#include <string_view>

#include "antwika/log/ILogger.hpp"
#include "antwika/log/Appender.hpp"

#ifdef _WIN32
#define LOG_EXPORT __declspec(dllexport)
#else
#define LOG_EXPORT
#endif

namespace antwika::log
{

    enum class Level
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
    };

    class LOG_EXPORT Logger : public ILogger
    {
    public:
        explicit Logger(Level level, Appender &appender);
        void trace(std::string_view message);
        void debug(std::string_view message);
        void info(std::string_view message);
        void warning(std::string_view message);
        void error(std::string_view message);
        void fatal(std::string_view message);

    private:
        Level level;
        Appender &appender;
        void log(std::string_view level, std::string_view message);
    };

} // namespace antwika::log
