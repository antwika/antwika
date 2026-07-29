#include <gtest/gtest.h>

#include "antwika/poker/AgentStyle.hpp"

using antwika::poker::AgentStyle;
using antwika::poker::toString;

TEST(AgentStyleTest, ToString_NamesEveryStyle)
{
    EXPECT_EQ(toString(AgentStyle::Tight), "tight");
    EXPECT_EQ(toString(AgentStyle::Balanced), "balanced");
    EXPECT_EQ(toString(AgentStyle::Aggressive), "aggressive");
}

TEST(AgentStyleTest, ToString_FallsBackForAValueThatNamesNoStyle)
{
    EXPECT_EQ(toString(static_cast<AgentStyle>(42)), "unknown");
}
