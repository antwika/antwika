#include "antwika/log/Logger.hpp"

#include <chrono>
#include <format>
#include <string>

namespace antwika::log
{

    Logger::Logger(IFormatter &formatter, antwika::time::IClock &clock, Level level, IAppender &appender) : formatter(formatter), clock(clock), level(level), appender(appender)
    {
    }

    void Logger::log(Level level, std::string_view message) noexcept
    {
        if (level < this->level)
            return;

        const auto now = clock.now();
        try
        {
            auto formatted = formatter.format(now, level, message);
            appender.append(formatted);
        }
        catch (...)
        {
            // Ignore, perhaps add a fallback
        }
    }

} // namespace antwika::log
