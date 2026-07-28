#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/time/fakes/FakeClock.hpp>
#include "antwika/log/mocks/MockFormatter.hpp"
#include "antwika/log/mocks/MockLogPolicy.hpp"
#include "antwika/log/mocks/MockAppender.hpp"

#include "antwika/log/Logger.hpp"

using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::mocks::MockAppender;
using antwika::log::mocks::MockFormatter;
using antwika::log::mocks::MockLogPolicy;
using antwika::time::fakes::FakeClock;

TEST(LoggerTest, log_WhenPolicyRejects_DoesNothing)
{
    MockFormatter mockFormatter;
    MockLogPolicy mockLogPolicy;
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    MockAppender mockAppender;
    Logger logger(mockFormatter, mockLogPolicy, fakeClock, mockAppender);

    EXPECT_CALL(mockLogPolicy, accepts(Level::Info))
        .WillOnce(::testing::Return(false));
    EXPECT_CALL(mockFormatter, format(::testing::_, ::testing::_, ::testing::_))
        .Times(0);
    EXPECT_CALL(mockAppender, append(::testing::_)).Times(0);

    logger.log(Level::Info, "Message");
}

TEST(LoggerTest, log_WhenPolicyAccepts_FormatsAndAppendsToAppender)
{
    MockFormatter mockFormatter;
    MockLogPolicy mockLogPolicy;
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    MockAppender mockAppender;
    Logger logger(mockFormatter, mockLogPolicy, fakeClock, mockAppender);

    EXPECT_CALL(mockLogPolicy, accepts(Level::Info))
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(mockFormatter, format(time, Level::Info, "Message"))
        .WillOnce(::testing::Return("Formatted message"));
    EXPECT_CALL(mockAppender, append("Formatted message"));

    logger.log(Level::Info, "Message");
}

TEST(LoggerTest, log_MustNotPropagateExceptionIfFormatterFails)
{
    MockFormatter mockFormatter;
    MockLogPolicy mockLogPolicy;
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    MockAppender mockAppender;
    Logger logger(mockFormatter, mockLogPolicy, fakeClock, mockAppender);

    EXPECT_CALL(mockLogPolicy, accepts(Level::Info))
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(mockFormatter, format(time, Level::Info, "Message"))
        .WillRepeatedly(::testing::Throw(std::exception{}));
    EXPECT_CALL(mockAppender, append(::testing::_)).Times(0);

    testing::internal::CaptureStderr();

    EXPECT_NO_THROW(logger.log(Level::Info, "Message"));

    const auto output = testing::internal::GetCapturedStderr();
    EXPECT_NE(output.find("Logger failure"), std::string::npos);
    EXPECT_NE(output.find("Message"), std::string::npos);
}

TEST(LoggerTest, log_MustNotPropagateExceptionIfAppenderFails)
{
    MockFormatter mockFormatter;
    MockLogPolicy mockLogPolicy;
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    MockAppender mockAppender;
    Logger logger(mockFormatter, mockLogPolicy, fakeClock, mockAppender);

    EXPECT_CALL(mockLogPolicy, accepts(Level::Info))
        .WillOnce(::testing::Return(true));
    EXPECT_CALL(mockFormatter, format(time, Level::Info, "Message"))
        .WillOnce(::testing::Return("Formatted message"));
    EXPECT_CALL(mockAppender, append(::testing::_))
        .WillRepeatedly(::testing::Throw(std::exception{}));

    testing::internal::CaptureStderr();

    EXPECT_NO_THROW(logger.log(Level::Info, "Message"));

    const auto output = testing::internal::GetCapturedStderr();
    EXPECT_NE(output.find("Logger failure"), std::string::npos);
    EXPECT_NE(output.find("Message"), std::string::npos);
}
