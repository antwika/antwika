#include <gtest/gtest.h>

#include "antwika/log/StreamAppender.hpp"

#include <sstream>

using antwika::log::StreamAppender;

TEST(StreamAppenderTest, Append_WritesMessageToStream) {
    std::ostringstream stream;
    StreamAppender appender(stream);
    appender.append("Message");
    EXPECT_EQ(stream.str(), "Message\n");
}
