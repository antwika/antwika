#pragma once

#include <string_view>

#include <antwika/time/IClock.hpp>

#include "antwika/log/IFormatter.hpp"
#include "antwika/log/ILogPolicy.hpp"
#include "antwika/log/ILogger.hpp"
#include "antwika/log/IAppender.hpp"
#include "antwika/log/Level.hpp"

namespace antwika::log
{

    class Logger : public ILogger
    {
    public:
        explicit Logger(IFormatter &formatter, ILogPolicy &policy, antwika::time::IClock &clock, IAppender &appender);
        void log(Level level, std::string_view message) noexcept;

    private:
        IFormatter &formatter;
        ILogPolicy &policy;
        antwika::time::IClock &clock;
        IAppender &appender;
    };

} // namespace antwika::log
