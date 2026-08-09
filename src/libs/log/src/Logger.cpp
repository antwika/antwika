#include "antwika/log/Logger.hpp"

#include <chrono>
#include <cstdio>

namespace antwika::log
{

    Logger::Logger(
        IFormatter &formatter,
        ILogPolicy &policy,
        IClock &clock,
        IAppender &appender)
        : formatter(formatter), policy(policy), clock(clock), appender(appender)
    {
    }

    void Logger::log(Level level, std::string_view message) noexcept
    {
        try
        {
            if (!policy.accepts(level))
            {
                return;
            }

            const auto now = clock.now();

            auto formatted = formatter.format(now, level, message);
            appender.append(formatted);
        }
        catch (...)
        {
            (void)std::fputs("Logger failure: ", stderr);
            (void)std::fwrite(message.data(), 1, message.size(), stderr);
            (void)std::fputc('\n', stderr);
        }
    }

}
