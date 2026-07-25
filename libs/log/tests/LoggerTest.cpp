#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/time/fakes/FakeClock.hpp>
#include "antwika/log/mocks/MockFormatter.hpp"
#include "antwika/log/mocks/MockAppender.hpp"

#include "antwika/log/Logger.hpp"

using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::mocks::MockFormatter;
using antwika::log::mocks::MockAppender;
using antwika::time::fakes::FakeClock;

#define EXPECT_LOG(level, call) [&] { \
    MockFormatter mockFormatter; \
    std::chrono::system_clock::time_point time{}; \
    FakeClock fakeClock(time); \
    MockAppender mockAppender; \
    Logger logger(mockFormatter, fakeClock, level, mockAppender); \
    EXPECT_CALL(mockFormatter, format(time, ::testing::_, "Message")).WillOnce(::testing::Return("Formatted message")); \
    EXPECT_CALL(mockAppender, append("Formatted message")); \
    call; }()

#define EXPECT_NO_LOG(level, call) [&] { \
    MockFormatter mockFormatter; \
    std::chrono::system_clock::time_point time{}; \
    FakeClock fakeClock(time); \
    MockAppender mockAppender; \
    Logger logger(mockFormatter, fakeClock, level, mockAppender); \
    EXPECT_CALL(mockFormatter, format(::testing::_, ::testing::_, ::testing::_)).Times(0); \
    EXPECT_CALL(mockAppender, append(::testing::_)).Times(0); \
    call; }()

TEST(LoggerTest, MustNotPropagateExceptionIfFormatterFails)
{
    MockFormatter mockFormatter;
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    MockAppender mockAppender;
    Logger logger(mockFormatter, fakeClock, Level::Trace, mockAppender);
    EXPECT_CALL(mockFormatter, format(time, ::testing::_, "Message")).WillRepeatedly(::testing::Throw(std::exception{}));
    EXPECT_NO_THROW(logger.trace("Message"));
    EXPECT_NO_THROW(logger.debug("Message"));
    EXPECT_NO_THROW(logger.warning("Message"));
    EXPECT_NO_THROW(logger.info("Message"));
    EXPECT_NO_THROW(logger.error("Message"));
    EXPECT_NO_THROW(logger.fatal("Message"));
}

TEST(LoggerTest, MustNotPropagateExceptionIfAppenderFails)
{
    MockFormatter mockFormatter;
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    MockAppender mockAppender;
    Logger logger(mockFormatter, fakeClock, Level::Trace, mockAppender);
    EXPECT_CALL(mockFormatter, format(time, ::testing::_, "Message")).WillRepeatedly(::testing::Return("Formatted message"));
    EXPECT_CALL(mockAppender, append("Formatted message")).WillRepeatedly(::testing::Throw(std::exception{}));
    EXPECT_NO_THROW(logger.trace("Message"));
    EXPECT_NO_THROW(logger.debug("Message"));
    EXPECT_NO_THROW(logger.warning("Message"));
    EXPECT_NO_THROW(logger.info("Message"));
    EXPECT_NO_THROW(logger.error("Message"));
    EXPECT_NO_THROW(logger.fatal("Message"));
}

TEST(LoggerTest, TraceIsEnabledWhenLoggerLevelIsTrace)
{
    EXPECT_LOG(Level::Trace, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Debug, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Info, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Error, logger.trace("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.trace("Message"));
}

TEST(LoggerTest, DebugIsEnabledWhenLoggerLevelIsTraceOrDebug)
{
    EXPECT_LOG(Level::Trace, logger.debug("Message"));
    EXPECT_LOG(Level::Debug, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Info, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Error, logger.debug("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.debug("Message"));
}

TEST(LoggerTest, InfoIsEnabledWhenLoggerLevelIsTraceDebugOrInfo)
{
    EXPECT_LOG(Level::Trace, logger.info("Message"));
    EXPECT_LOG(Level::Debug, logger.info("Message"));
    EXPECT_LOG(Level::Info, logger.info("Message"));
    EXPECT_NO_LOG(Level::Warning, logger.info("Message"));
    EXPECT_NO_LOG(Level::Error, logger.info("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.info("Message"));
}

TEST(LoggerTest, WarningIsEnabledWhenLoggerLevelIsTraceDebugInfoOrWarning)
{
    EXPECT_LOG(Level::Trace, logger.warning("Message"));
    EXPECT_LOG(Level::Debug, logger.warning("Message"));
    EXPECT_LOG(Level::Info, logger.warning("Message"));
    EXPECT_LOG(Level::Warning, logger.warning("Message"));
    EXPECT_NO_LOG(Level::Error, logger.warning("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.warning("Message"));
}

TEST(LoggerTest, ErrorIsEnabledWhenLoggerLevelIsTraceDebugInfoWarningOrError)
{
    EXPECT_LOG(Level::Trace, logger.error("Message"));
    EXPECT_LOG(Level::Debug, logger.error("Message"));
    EXPECT_LOG(Level::Info, logger.error("Message"));
    EXPECT_LOG(Level::Warning, logger.error("Message"));
    EXPECT_LOG(Level::Error, logger.error("Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.error("Message"));
}

TEST(LoggerTest, FatalIsEnabledWhenLoggerLevelIsTraceDebugInfoWarningErrorOrFatal)
{
    EXPECT_LOG(Level::Trace, logger.fatal("Message"));
    EXPECT_LOG(Level::Debug, logger.fatal("Message"));
    EXPECT_LOG(Level::Info, logger.fatal("Message"));
    EXPECT_LOG(Level::Warning, logger.fatal("Message"));
    EXPECT_LOG(Level::Error, logger.fatal("Message"));
    EXPECT_LOG(Level::Fatal, logger.fatal("Message"));
}
