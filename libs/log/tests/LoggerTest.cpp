#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/time/fakes/FakeClock.hpp>
#include "antwika/log/mocks/MockFormatter.hpp"
#include "antwika/log/mocks/MockAppender.hpp"

#include "antwika/log/Logger.hpp"

using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::mocks::MockAppender;
using antwika::log::mocks::MockFormatter;
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
    EXPECT_NO_THROW(logger.log(Level::Trace, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Debug, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Warning, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Info, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Error, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Fatal, "Message"));
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
    EXPECT_NO_THROW(logger.log(Level::Trace, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Debug, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Warning, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Info, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Error, "Message"));
    EXPECT_NO_THROW(logger.log(Level::Fatal, "Message"));
}

TEST(LoggerTest, TraceIsEnabledWhenLoggerLevelIsTrace)
{
    EXPECT_LOG(Level::Trace, logger.log(Level::Trace, "Message"));
    EXPECT_NO_LOG(Level::Debug, logger.log(Level::Trace, "Message"));
    EXPECT_NO_LOG(Level::Info, logger.log(Level::Trace, "Message"));
    EXPECT_NO_LOG(Level::Warning, logger.log(Level::Trace, "Message"));
    EXPECT_NO_LOG(Level::Error, logger.log(Level::Trace, "Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.log(Level::Trace, "Message"));
}

TEST(LoggerTest, DebugIsEnabledWhenLoggerLevelIsTraceOrDebug)
{
    EXPECT_LOG(Level::Trace, logger.log(Level::Debug, "Message"));
    EXPECT_LOG(Level::Debug, logger.log(Level::Debug, "Message"));
    EXPECT_NO_LOG(Level::Info, logger.log(Level::Debug, "Message"));
    EXPECT_NO_LOG(Level::Warning, logger.log(Level::Debug, "Message"));
    EXPECT_NO_LOG(Level::Error, logger.log(Level::Debug, "Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.log(Level::Debug, "Message"));
}

TEST(LoggerTest, InfoIsEnabledWhenLoggerLevelIsTraceDebugOrInfo)
{
    EXPECT_LOG(Level::Trace, logger.log(Level::Info, "Message"));
    EXPECT_LOG(Level::Debug, logger.log(Level::Info, "Message"));
    EXPECT_LOG(Level::Info, logger.log(Level::Info, "Message"));
    EXPECT_NO_LOG(Level::Warning, logger.log(Level::Info, "Message"));
    EXPECT_NO_LOG(Level::Error, logger.log(Level::Info, "Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.log(Level::Info, "Message"));
}

TEST(LoggerTest, WarningIsEnabledWhenLoggerLevelIsTraceDebugInfoOrWarning)
{
    EXPECT_LOG(Level::Trace, logger.log(Level::Warning, "Message"));
    EXPECT_LOG(Level::Debug, logger.log(Level::Warning, "Message"));
    EXPECT_LOG(Level::Info, logger.log(Level::Warning, "Message"));
    EXPECT_LOG(Level::Warning, logger.log(Level::Warning, "Message"));
    EXPECT_NO_LOG(Level::Error, logger.log(Level::Warning, "Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.log(Level::Warning, "Message"));
}

TEST(LoggerTest, ErrorIsEnabledWhenLoggerLevelIsTraceDebugInfoWarningOrError)
{
    EXPECT_LOG(Level::Trace, logger.log(Level::Error, "Message"));
    EXPECT_LOG(Level::Debug, logger.log(Level::Error, "Message"));
    EXPECT_LOG(Level::Info, logger.log(Level::Error, "Message"));
    EXPECT_LOG(Level::Warning, logger.log(Level::Error, "Message"));
    EXPECT_LOG(Level::Error, logger.log(Level::Error, "Message"));
    EXPECT_NO_LOG(Level::Fatal, logger.log(Level::Error, "Message"));
}

TEST(LoggerTest, FatalIsEnabledWhenLoggerLevelIsTraceDebugInfoWarningErrorOrFatal)
{
    EXPECT_LOG(Level::Trace, logger.log(Level::Fatal, "Message"));
    EXPECT_LOG(Level::Debug, logger.log(Level::Fatal, "Message"));
    EXPECT_LOG(Level::Info, logger.log(Level::Fatal, "Message"));
    EXPECT_LOG(Level::Warning, logger.log(Level::Fatal, "Message"));
    EXPECT_LOG(Level::Error, logger.log(Level::Fatal, "Message"));
    EXPECT_LOG(Level::Fatal, logger.log(Level::Fatal, "Message"));
}
