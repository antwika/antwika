#pragma once

#include <string_view>

#include <antwika/time/IClock.hpp>

#include "IFormatter.hpp"
#include "ILogPolicy.hpp"
#include "ILogger.hpp"
#include "IAppender.hpp"
#include "Level.hpp"

using antwika::time::IClock;

namespace antwika::log
{

    class Logger : public ILogger
    {
    public:
        explicit Logger(IFormatter &formatter, ILogPolicy &policy, IClock &clock, IAppender &appender);
        void log(Level level, std::string_view message) noexcept;

    private:
        IFormatter &formatter;
        ILogPolicy &policy;
        IClock &clock;
        IAppender &appender;
    };

} // namespace antwika::log
