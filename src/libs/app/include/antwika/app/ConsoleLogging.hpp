#pragma once

#include <ostream>

#include <antwika/log/ILogger.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/time/SystemClock.hpp>

namespace antwika::app
{

    using antwika::log::ILogger;
    using antwika::log::Level;

    class ConsoleLogging final
    {
    public:
        explicit ConsoleLogging(std::ostream &outputStream, Level minimumLevel);

        ConsoleLogging(const ConsoleLogging &) = delete;
        ConsoleLogging(ConsoleLogging &&) = delete;

        ConsoleLogging &operator=(const ConsoleLogging &) = delete;
        ConsoleLogging &operator=(ConsoleLogging &&) = delete;

        [[nodiscard]] ILogger &logger() noexcept;

    private:
        antwika::time::SystemClock clock;
        antwika::log::StreamAppender appender;
        antwika::log::PlainFormatter formatter;
        antwika::log::MinimumLevelLogPolicy policy;
        antwika::log::Logger consoleLogger;
    };

}
