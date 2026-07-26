#include "antwika/log/Logger.hpp"

#include <chrono>
#include <format>
#include <string>

namespace antwika::log
{

    Logger::Logger(IFormatter &formatter, ILogPolicy &policy, IClock &clock, IAppender &appender) : formatter(formatter), policy(policy), clock(clock), appender(appender)
    {
    }

    void Logger::log(Level level, std::string_view message) noexcept
    {
        try
        {
            if (!policy.accepts(level))
                return;

            const auto now = clock.now();

            auto formatted = formatter.format(now, level, message);
            appender.append(formatted);
        }
        catch (...)
        {
            // Ignore, perhaps add a fallback
        }
    }

} // namespace antwika::log
