#pragma once

#include <string_view>

#include <antwika/time/IClock.hpp>

#include "antwika/log/ILogger.hpp"
#include "antwika/log/Appender.hpp"

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

    class Logger : public ILogger
    {
    public:
        explicit Logger(antwika::time::IClock &clock, Level level, Appender &appender);
        void trace(std::string_view message) noexcept;
        void debug(std::string_view message) noexcept;
        void info(std::string_view message) noexcept;
        void warning(std::string_view message) noexcept;
        void error(std::string_view message) noexcept;
        void fatal(std::string_view message) noexcept;

    private:
        antwika::time::IClock &clock;
        Level level;
        Appender &appender;
        void log(std::string_view level, std::string_view message) noexcept;
    };

} // namespace antwika::log
