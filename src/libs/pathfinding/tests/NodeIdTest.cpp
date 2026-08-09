#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/pathfinding/NodeId.hpp>

namespace
{

    using antwika::pathfinding::nodeId;
    using antwika::pathfinding::NodeId;
    using antwika::pathfinding::rawValue;

}

TEST(NodeIdTest, RawValue_UnwrapsWhatNodeIdWrapped)
{
    EXPECT_EQ(rawValue(nodeId(0)), 0U);
    EXPECT_EQ(rawValue(nodeId(4294967295U)), 4294967295U);
}

TEST(NodeIdTest, NodeId_OrdersByTheNumberItCarries)
{
    constexpr std::uint32_t kLastInt32 = 2147483647U;
    constexpr std::uint32_t kPastInt32 = 2147483648U;

    EXPECT_LT(nodeId(1), nodeId(2));
    EXPECT_LT(nodeId(kLastInt32), nodeId(kPastInt32));
    EXPECT_GT(nodeId(kPastInt32), nodeId(0));
}

TEST(NodeIdTest, NodeId_IsConstexpr)
{
    static_assert(rawValue(nodeId(7)) == 7U);
    static_assert(nodeId(7) == static_cast<NodeId>(std::uint32_t{7}));

    SUCCEED();
}
