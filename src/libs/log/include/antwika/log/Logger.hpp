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

    /**
     * @brief ILogger that formats and appends records that pass a policy check.
     */
    class Logger : public ILogger
    {
    public:
        /**
         * @brief Construct a logger from its collaborators.
         * @param formatter Renders each accepted record into text.
         * @param policy Decides which records are accepted.
         * @param clock Supplies the timestamp attached to each record.
         * @param appender Receives the formatted text of each accepted record.
         */
        explicit Logger(
            IFormatter &formatter,
            ILogPolicy &policy,
            IClock &clock,
            IAppender &appender);

        Logger(const Logger &) = delete;
        Logger(Logger &&) = delete;

        Logger &operator=(const Logger &) = delete;
        Logger &operator=(Logger &&) = delete;

        /**
         * @brief Log a message if the configured policy accepts its level.
         * @param level The severity level of the message.
         * @param message The message to log.
         */
        void log(Level level, std::string_view message) noexcept override;

    private:
        IFormatter &formatter;
        ILogPolicy &policy;
        IClock &clock;
        IAppender &appender;
    };

} // namespace antwika::log
