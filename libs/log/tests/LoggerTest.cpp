#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/time/fakes/FakeClock.hpp>
#include "antwika/log/mocks/MockAppender.hpp"

#include "antwika/log/Logger.hpp"

using antwika::time::fakes::FakeClock;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::mocks::MockAppender;

#define EXPECT_LOG(level, expected, call) [&] { \
    FakeClock fakeClock(std::chrono::system_clock::from_time_t(0)); \
    MockAppender mockAppender; \
    Logger logger(fakeClock, level, mockAppender); \
    EXPECT_CALL(mockAppender, append(expected)); \
    call; }()

#define EXPECT_NO_LOG(level, call) [&] { \
    FakeClock fakeClock(std::chrono::system_clock::from_time_t(0)); \
    MockAppender mockAppender; \
    Logger logger(fakeClock, level, mockAppender); \
    EXPECT_CALL(mockAppender, append(::testing::_)).Times(0); \
    call; }()

TEST(LoggerTest, TraceIsEnabledWhenLoggerLevelIsTrace)
{
    EXPECT_LOG(Level::Trace, "[1970-01-01 00:00:00] [TRACE] Message", logger.trace("Message"));
    EXPECT_NO_LOG(Level::Debug, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Info, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Error, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.trace("Message"));
}

TEST(LoggerTest, DebugIsEnabledWhenLoggerLevelIsTraceOrDebug)
{
    EXPECT_LOG(Level::Trace, "[1970-01-01 00:00:00] [DEBUG] Message", logger.debug("Message"));
    EXPECT_LOG(Level::Debug, "[1970-01-01 00:00:00] [DEBUG] Message", logger.debug("Message"));
    EXPECT_NO_LOG(Level::Info, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Error, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.debug("Message"));
}

TEST(LoggerTest, InfoIsEnabledWhenLoggerLevelIsTraceDebugOrInfo)
{
    EXPECT_LOG(Level::Trace, "[1970-01-01 00:00:00] [INFO] Message", logger.info("Message"));
    EXPECT_LOG(Level::Debug, "[1970-01-01 00:00:00] [INFO] Message", logger.info("Message"));
    EXPECT_LOG(Level::Info, "[1970-01-01 00:00:00] [INFO] Message", logger.info("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.info("Message"));
    EXPECT_NO_LOG(Level::Error, logger.info("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.info("Message"));
}

TEST(LoggerTest, WarningIsEnabledWhenLoggerLevelIsTraceDebugInfoOrWarning)
{
    EXPECT_LOG(Level::Trace, "[1970-01-01 00:00:00] [WARNING] Message", logger.warning("Message"));
    EXPECT_LOG(Level::Debug, "[1970-01-01 00:00:00] [WARNING] Message", logger.warning("Message"));
    EXPECT_LOG(Level::Info, "[1970-01-01 00:00:00] [WARNING] Message", logger.warning("Message"));
    EXPECT_LOG(Level::Warning, "[1970-01-01 00:00:00] [WARNING] Message", logger.warning("Message"));
    EXPECT_NO_LOG(Level::Error, logger.warning("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.warning("Message"));
}

TEST(LoggerTest, ErrorIsEnabledWhenLoggerLevelIsTraceDebugInfoWarningOrError)
{
    EXPECT_LOG(Level::Trace, "[1970-01-01 00:00:00] [ERROR] Message", logger.error("Message"));
    EXPECT_LOG(Level::Debug, "[1970-01-01 00:00:00] [ERROR] Message", logger.error("Message"));
    EXPECT_LOG(Level::Info, "[1970-01-01 00:00:00] [ERROR] Message", logger.error("Message"));
    EXPECT_LOG(Level::Warning, "[1970-01-01 00:00:00] [ERROR] Message", logger.error("Message"));
    EXPECT_LOG(Level::Error, "[1970-01-01 00:00:00] [ERROR] Message", logger.error("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.error("Message"));
}

TEST(LoggerTest, FatalIsEnabledWhenLoggerLevelIsTraceDebugInfoWarningErrorOrFatal)
{
    EXPECT_LOG(Level::Trace, "[1970-01-01 00:00:00] [FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Debug, "[1970-01-01 00:00:00] [FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Info, "[1970-01-01 00:00:00] [FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Warning, "[1970-01-01 00:00:00] [FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Error, "[1970-01-01 00:00:00] [FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Fatal, "[1970-01-01 00:00:00] [FATAL] Message", logger.fatal("Message"));
}
