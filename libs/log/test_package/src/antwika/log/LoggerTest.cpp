#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "antwika/log/mocks/MockAppender.hpp"

#include "antwika/log/Logger.hpp"

using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::mocks::MockAppender;

#define EXPECT_LOG(level, expected, call) [&] { \
    MockAppender mockAppender; \
    Logger logger(level, mockAppender); \
    EXPECT_CALL(mockAppender, append(expected)); \
    call; }()

#define EXPECT_NO_LOG(level, call) [&] { \
    MockAppender mockAppender; \
    Logger logger(level, mockAppender); \
    EXPECT_CALL(mockAppender, append(::testing::_)).Times(0); \
    call; }()

TEST(LoggerTest, TraceIsEnabledWhenLoggerLevelIsTrace)
{
    EXPECT_LOG(Level::Trace, "[TRACE] Message", logger.trace("Message"));
    EXPECT_NO_LOG(Level::Debug, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Info, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Error, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.trace("Message"));
}

TEST(LoggerTest, DebugIsEnabledWhenLoggerLevelIsTraceOrDebug)
{
    EXPECT_LOG(Level::Trace, "[DEBUG] Message", logger.debug("Message"));
    EXPECT_LOG(Level::Debug, "[DEBUG] Message", logger.debug("Message"));
    EXPECT_NO_LOG(Level::Info, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Error, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.debug("Message"));
}

TEST(LoggerTest, InfoIsEnabledWhenLoggerLevelIsTraceDebugOrInfo)
{
    EXPECT_LOG(Level::Trace, "[INFO] Message", logger.info("Message"));
    EXPECT_LOG(Level::Debug, "[INFO] Message", logger.info("Message"));
    EXPECT_LOG(Level::Info, "[INFO] Message", logger.info("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.info("Message"));
    EXPECT_NO_LOG(Level::Error, logger.info("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.info("Message"));
}

TEST(LoggerTest, WarningIsEnabledWhenLoggerLevelIsTraceDebugInfoOrWarning)
{
    EXPECT_LOG(Level::Trace, "[WARNING] Message", logger.warning("Message"));
    EXPECT_LOG(Level::Debug, "[WARNING] Message", logger.warning("Message"));
    EXPECT_LOG(Level::Info, "[WARNING] Message", logger.warning("Message"));
    EXPECT_LOG(Level::Warning, "[WARNING] Message", logger.warning("Message"));
    EXPECT_NO_LOG(Level::Error, logger.warning("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.warning("Message"));
}

TEST(LoggerTest, ErrorIsEnabledWhenLoggerLevelIsTraceDebugInfoWarningOrError)
{
    EXPECT_LOG(Level::Trace, "[ERROR] Message", logger.error("Message"));
    EXPECT_LOG(Level::Debug, "[ERROR] Message", logger.error("Message"));
    EXPECT_LOG(Level::Info, "[ERROR] Message", logger.error("Message"));
    EXPECT_LOG(Level::Warning, "[ERROR] Message", logger.error("Message"));
    EXPECT_LOG(Level::Error, "[ERROR] Message", logger.error("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.error("Message"));
}

TEST(LoggerTest, FatalIsEnabledWhenLoggerLevelIsTraceDebugInfoWarningErrorOrFatal)
{
    EXPECT_LOG(Level::Trace, "[FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Debug, "[FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Info, "[FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Warning, "[FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Error, "[FATAL] Message", logger.fatal("Message"));
    EXPECT_LOG(Level::Fatal, "[FATAL] Message", logger.fatal("Message"));
}
