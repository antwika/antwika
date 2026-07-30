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

    /**
     * @brief A logger writing plain lines to a stream, and the four
     * collaborators it is built out of.
     *
     * Every application wants the same five objects in the same order,
     * and each one used to declare all five itself. Declaring them is
     * not the interesting part; the order is: a Logger holds references
     * to the other four, so they have to outlive it, and a member is
     * destroyed in reverse declaration order. Owning them together is
     * what makes that order a property of this class rather than of five
     * lines somebody may reorder.
     *
     * One appender gets one logger, which is the guarantee the apps'
     * bootstrap() functions used to state in prose: two loggers over one
     * appender interleave their lines.
     */
    class ConsoleLogging final
    {
    public:
        /**
         * @brief Construct the logger and everything under it.
         * @param out Stream every log line is written to; must outlive
         * this object.
         * @param minimum Lowest level that is emitted at all.
         */
        explicit ConsoleLogging(std::ostream &out, Level minimum);

        ConsoleLogging(const ConsoleLogging &) = delete;
        ConsoleLogging(ConsoleLogging &&) = delete;

        ConsoleLogging &operator=(const ConsoleLogging &) = delete;
        ConsoleLogging &operator=(ConsoleLogging &&) = delete;

        /**
         * @brief Get the one logger over this stream.
         * @return The logger.
         */
        [[nodiscard]] ILogger &logger() noexcept;

    private:
        // Declared in the order they have to be constructed in.
        // The logger references the other four, so it comes last.
        antwika::time::SystemClock clock;
        antwika::log::StreamAppender appender;
        antwika::log::PlainFormatter formatter;
        antwika::log::MinimumLevelLogPolicy policy;
        antwika::log::Logger consoleLogger;
    };

} // namespace antwika::app
