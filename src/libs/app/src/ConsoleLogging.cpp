#include "antwika/app/ConsoleLogging.hpp"

namespace antwika::app
{

    ConsoleLogging::ConsoleLogging(std::ostream &out, Level minimum)
        : appender(out),
          policy(minimum),
          consoleLogger(formatter, policy, clock, appender)
    {
    }

    ILogger &ConsoleLogging::logger() noexcept
    {
        return consoleLogger;
    }

} // namespace antwika::app
