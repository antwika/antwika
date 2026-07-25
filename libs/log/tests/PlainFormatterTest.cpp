#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "antwika/log/PlainFormatter.hpp"

using antwika::log::PlainFormatter;

TEST(PlainFormatterTest, Append)
{
    std::chrono::system_clock::time_point time{};
    std::string level = "INFO";
    std::string message = "Message";
    PlainFormatter formatter;
    auto formatted = formatter.format(time, level, message);
    EXPECT_EQ(formatted, "[1970-01-01 00:00:00] [INFO] Message");
}
