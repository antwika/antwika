#include "antwika/ecs/Entity.hpp"

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

using antwika::ecs::Entity;
using antwika::ecs::entityGeneration;
using antwika::ecs::entityIndex;
using antwika::ecs::kEntityIndexBits;
using antwika::ecs::kMaxEntityGeneration;
using antwika::ecs::kMaxEntityIndex;
using antwika::ecs::kNullEntity;
using antwika::ecs::makeEntity;
using antwika::ecs::rawValue;

TEST(EntityTest, KNullEntityIsZeroInBothParts)
{
    EXPECT_EQ(rawValue(kNullEntity), 0U);
    EXPECT_EQ(entityIndex(kNullEntity), 0U);
    EXPECT_EQ(entityGeneration(kNullEntity), 0U);
}

TEST(EntityTest, MakeEntityRoundTripsBothParts)
{
    const auto entity = makeEntity(42, 7);

    EXPECT_EQ(entityIndex(entity), 42U);
    EXPECT_EQ(entityGeneration(entity), 7U);
}

TEST(EntityTest, TheTwoPartsDoNotBleedIntoEachOther)
{
    const auto entity = makeEntity(kMaxEntityIndex, kMaxEntityGeneration);

    EXPECT_EQ(entityIndex(entity), kMaxEntityIndex);
    EXPECT_EQ(entityGeneration(entity), kMaxEntityGeneration);
    EXPECT_EQ(rawValue(entity), std::numeric_limits<std::uint64_t>::max());
}

TEST(EntityTest, GenerationZeroLeavesTheRawValueEqualToTheIndex)
{
    // Every handle a world hands out first is generation 0.
    // Which is what keeps a plain Entity{n} meaning slot n.
    EXPECT_EQ(rawValue(makeEntity(9, 0)), 9U);
    EXPECT_EQ(entityIndex(Entity{9}), 9U);
}

TEST(EntityTest, TheSplitLeavesRoomForBothHalves)
{
    static_assert(kEntityIndexBits > 0);
    static_assert(kEntityIndexBits < 64);
    static_assert(kMaxEntityIndex > 0);
    static_assert(kMaxEntityGeneration > 0);

    SUCCEED();
}
