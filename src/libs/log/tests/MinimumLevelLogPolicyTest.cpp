#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "antwika/log/MinimumLevelLogPolicy.hpp"

using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;

TEST(MinimumLevelLogPolicyTest, Accepts_WhenLogLevelIsTrace)
{
    MinimumLevelLogPolicy minimumLevelLogPolicy(Level::Trace);
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Trace));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Debug));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Info));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Warning));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Error));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Fatal));
}

TEST(MinimumLevelLogPolicyTest, Accepts_WhenLogLevelIsDebug)
{
    MinimumLevelLogPolicy minimumLevelLogPolicy(Level::Debug);
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Trace));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Debug));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Info));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Warning));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Error));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Fatal));
}

TEST(MinimumLevelLogPolicyTest, Accepts_WhenLogLevelIsInfo)
{
    MinimumLevelLogPolicy minimumLevelLogPolicy(Level::Info);
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Trace));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Debug));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Info));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Warning));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Error));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Fatal));
}

TEST(MinimumLevelLogPolicyTest, Accepts_WhenLogLevelIsWarning)
{
    MinimumLevelLogPolicy minimumLevelLogPolicy(Level::Warning);
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Trace));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Debug));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Info));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Warning));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Error));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Fatal));
}

TEST(MinimumLevelLogPolicyTest, Accepts_WhenLogLevelIsError)
{
    MinimumLevelLogPolicy minimumLevelLogPolicy(Level::Error);
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Trace));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Debug));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Info));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Warning));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Error));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Fatal));
}

TEST(MinimumLevelLogPolicyTest, Accepts_WhenLogLevelIsFatal)
{
    MinimumLevelLogPolicy minimumLevelLogPolicy(Level::Fatal);
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Trace));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Debug));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Info));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Warning));
    EXPECT_FALSE(minimumLevelLogPolicy.accepts(Level::Error));
    EXPECT_TRUE(minimumLevelLogPolicy.accepts(Level::Fatal));
}
