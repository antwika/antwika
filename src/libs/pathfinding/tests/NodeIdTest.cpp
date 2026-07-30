#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/pathfinding/NodeId.hpp>

namespace
{

    using antwika::pathfinding::nodeId;
    using antwika::pathfinding::NodeId;
    using antwika::pathfinding::rawValue;

} // namespace

TEST(NodeIdTest, RawValue_UnwrapsWhatNodeIdWrapped)
{
    EXPECT_EQ(rawValue(nodeId(0)), 0U);
    EXPECT_EQ(rawValue(nodeId(4294967295U)), 4294967295U);
}

TEST(NodeIdTest, NodeId_OrdersByTheNumberItCarries)
{
    EXPECT_LT(nodeId(1), nodeId(2));
    EXPECT_EQ(nodeId(3), nodeId(3));
}

TEST(NodeIdTest, NodeId_IsConstexpr)
{
    static_assert(rawValue(nodeId(7)) == 7U);
    static_assert(nodeId(7) == static_cast<NodeId>(std::uint32_t{7}));

    SUCCEED();
}
