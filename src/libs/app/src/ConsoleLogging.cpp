#include "antwika/app/ConsoleLogging.hpp"

namespace antwika::app
{

    ConsoleLogging::ConsoleLogging(
        std::ostream &outputStream, Level minimumLevel)
        : appender(outputStream),
          policy(minimumLevel),
          consoleLogger(formatter, policy, clock, appender)
    {
    }

    ILogger &ConsoleLogging::logger() noexcept
    {
        return consoleLogger;
    }

}
