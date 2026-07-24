#include <gtest/gtest.h>

#include "antwika/log/NullAppender.hpp"

using antwika::log::NullAppender;

TEST(NullAppenderTest, Append)
{
    NullAppender appender;
    appender.append("Message");
}
