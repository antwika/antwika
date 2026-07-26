#pragma once

#include <string_view>

#include <antwika/time/IClock.hpp>

#include "IFormatter.hpp"
#include "ILogPolicy.hpp"
#include "ILogger.hpp"
#include "IAppender.hpp"
#include "Level.hpp"

namespace antwika::log
{

    using antwika::time::IClock;

    class Logger : public ILogger
    {
    public:
        explicit Logger(IFormatter &formatter, ILogPolicy &policy, IClock &clock, IAppender &appender);

        Logger(const Logger &) = delete;
        Logger(Logger &&) = delete;

        Logger &operator=(const Logger &) = delete;
        Logger &operator=(Logger &&) = delete;

        void log(Level level, std::string_view message) noexcept override;

    private:
        IFormatter &formatter;
        ILogPolicy &policy;
        IClock &clock;
        IAppender &appender;
    };

} // namespace antwika::log
