#include <gtest/gtest.h>

#include <string>

#include "antwika/log/NullAppender.hpp"

using antwika::log::NullAppender;

TEST(NullAppenderTest, Append_DiscardsTheMessage)
{
    NullAppender appender;

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();

    appender.append("Message");

    EXPECT_EQ(testing::internal::GetCapturedStdout(), std::string{});
    EXPECT_EQ(testing::internal::GetCapturedStderr(), std::string{});
}
