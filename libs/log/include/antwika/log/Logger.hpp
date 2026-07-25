#pragma once

#include <string_view>

#include <antwika/time/IClock.hpp>

#include "antwika/log/IFormatter.hpp"
#include "antwika/log/ILogger.hpp"
#include "antwika/log/IAppender.hpp"

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
        explicit Logger(IFormatter& formatter, antwika::time::IClock &clock, Level level, IAppender &appender);
        void trace(std::string_view message) noexcept;
        void debug(std::string_view message) noexcept;
        void info(std::string_view message) noexcept;
        void warning(std::string_view message) noexcept;
        void error(std::string_view message) noexcept;
        void fatal(std::string_view message) noexcept;

    private:
        IFormatter &formatter;
        antwika::time::IClock &clock;
        Level level;
        IAppender &appender;
        void log(std::string_view level, std::string_view message) noexcept;
    };

} // namespace antwika::log
