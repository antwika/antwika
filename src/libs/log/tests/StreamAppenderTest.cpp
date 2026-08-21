#include <gtest/gtest.h>

#include <sstream>

#include "antwika/log/StreamAppender.hpp"

using antwika::log::StreamAppender;

TEST(StreamAppenderTest, Append_WritesMessageToStream)
{
    std::ostringstream stream;
    StreamAppender appender(stream);
    appender.append("Message");
    EXPECT_EQ(stream.str(), "Message\n");
}
