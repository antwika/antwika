#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
