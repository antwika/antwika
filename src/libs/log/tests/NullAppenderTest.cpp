#include <gtest/gtest.h>

#include "antwika/log/NullAppender.hpp"

using antwika::log::NullAppender;

TEST(NullAppenderTest, Append_DiscardsTheMessage)
{
    NullAppender appender;
    appender.append("Message");
}
