#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "antwika/log/PlainFormatter.hpp"
#include "antwika/log/Level.hpp"

using antwika::log::PlainFormatter;

TEST(PlainFormatterTest, Format_RendersTheTimestampLevelAndMessage)
{
    std::chrono::system_clock::time_point time{};
    std::string message = "Message";
    PlainFormatter formatter;
    auto formatted = formatter.format(time, antwika::log::Level::Info, message);
    EXPECT_EQ(formatted, "[1970-01-01 00:00:00] [INFO] Message");
}

TEST(PlainFormatterTest, Format_CutsTheClockBackToWholeSeconds)
{
    const auto time = std::chrono::system_clock::time_point{}
        + std::chrono::seconds{1700000000}
        + std::chrono::milliseconds{750};
    const PlainFormatter formatter;

    EXPECT_EQ(
        formatter.format(time, antwika::log::Level::Warning, "Message"),
        "[2023-11-14 22:13:20] [WARNING] Message");
}
