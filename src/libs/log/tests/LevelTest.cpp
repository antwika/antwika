#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/log/Level.hpp"

using antwika::log::Level;

TEST(LevelTest, ToString_NamesEveryLevelAndAnythingElse)
{
    EXPECT_EQ(toString(Level::Trace), "TRACE");
    EXPECT_EQ(toString(Level::Debug), "DEBUG");
    EXPECT_EQ(toString(Level::Warning), "WARNING");
    EXPECT_EQ(toString(Level::Info), "INFO");
    EXPECT_EQ(toString(Level::Error), "ERROR");
    EXPECT_EQ(toString(Level::Fatal), "FATAL");
    EXPECT_EQ(toString(static_cast<Level>(-1)), "UNKNOWN");
}

TEST(LevelTest, Level_NumbersTheScaleInStepsOfTen)
{
    EXPECT_EQ(static_cast<std::uint8_t>(Level::Trace), 0U);
    EXPECT_EQ(static_cast<std::uint8_t>(Level::Debug), 10U);
    EXPECT_EQ(static_cast<std::uint8_t>(Level::Info), 20U);
    EXPECT_EQ(static_cast<std::uint8_t>(Level::Warning), 30U);
    EXPECT_EQ(static_cast<std::uint8_t>(Level::Error), 40U);
    EXPECT_EQ(static_cast<std::uint8_t>(Level::Fatal), 50U);
}
