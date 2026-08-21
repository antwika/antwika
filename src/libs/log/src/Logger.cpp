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

            const auto nowTime = clock.now();

            auto formattedLine = formatter.format(nowTime, level, message);
            appender.append(formattedLine);
        }
        catch (...)
        {
            (void)std::fputs("Logger failure: ", stderr);
            (void)std::fwrite(message.data(), 1, message.size(), stderr);
            (void)std::fputc('\n', stderr);
        }
    }

}
