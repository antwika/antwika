#include <cstdint>
#include <limits>
#include <map>
#include <type_traits>

#include <gtest/gtest.h>

#include <antwika/ecs/Component.hpp>

#include "antwika/ecs_commons/GridPosition.hpp"
#include "antwika/ecs_commons/Lifetime.hpp"
#include "antwika/ecs_commons/Name.hpp"
#include "antwika/ecs_commons/Tag.hpp"
#include "antwika/ecs_commons/Velocity.hpp"

using antwika::ecs_commons::GridPosition;
using antwika::ecs_commons::Lifetime;
using antwika::ecs_commons::Name;
using antwika::ecs_commons::stepBy;
using antwika::ecs_commons::Tag;
using antwika::ecs_commons::Velocity;

namespace
{
    using Path = Tag<struct PathKind>;
    using Blocked = Tag<struct BlockedKind>;
} // namespace

static_assert(antwika::ecs::Component<GridPosition>);
static_assert(antwika::ecs::Component<Velocity>);
static_assert(antwika::ecs::Component<Lifetime>);
static_assert(antwika::ecs::Component<Name>);
static_assert(antwika::ecs::Component<Path>);

TEST(GridPositionTest, DefaultsToTheOrigin)
{
    const GridPosition position{};

    EXPECT_EQ(position.x, 0);
    EXPECT_EQ(position.y, 0);
}

TEST(GridPositionTest, ComparesByBothCoordinates)
{
    EXPECT_EQ((GridPosition{.x = 3, .y = -4}), (GridPosition{.x = 3, .y = -4}));
    EXPECT_NE((GridPosition{.x = 3, .y = -4}), (GridPosition{.x = 3, .y = 4}));
    EXPECT_NE(
        (GridPosition{.x = 3, .y = -4}), (GridPosition{.x = -3, .y = -4}));
}

TEST(GridPositionTest, OrdersLexicographicallyByXThenY)
{
    EXPECT_LT((GridPosition{.x = 1, .y = 9}), (GridPosition{.x = 2, .y = 0}));
    EXPECT_LT((GridPosition{.x = 1, .y = 0}), (GridPosition{.x = 1, .y = 1}));
    EXPECT_GT((GridPosition{.x = 2, .y = 0}), (GridPosition{.x = 1, .y = 9}));
    EXPECT_GT((GridPosition{.x = 1, .y = 1}), (GridPosition{.x = 1, .y = 0}));
    EXPECT_LE((GridPosition{.x = 1, .y = 1}), (GridPosition{.x = 1, .y = 1}));
    EXPECT_GE((GridPosition{.x = 1, .y = 1}), (GridPosition{.x = 1, .y = 1}));
}

TEST(GridPositionTest, CanKeyAnOrderedMapWithoutAComparator)
{
    std::map<GridPosition, int> byPosition;
    byPosition[GridPosition{.x = 2, .y = 0}] = 20;
    byPosition[GridPosition{.x = 1, .y = 5}] = 15;

    EXPECT_EQ(byPosition.begin()->first, (GridPosition{.x = 1, .y = 5}));
}

TEST(VelocityTest, DefaultsToStandingStill)
{
    const Velocity velocity{};

    EXPECT_EQ(velocity, (Velocity{.dx = 0, .dy = 0}));
}

TEST(VelocityTest, ComparesByBothComponents)
{
    EXPECT_EQ((Velocity{.dx = 1, .dy = 2}), (Velocity{.dx = 1, .dy = 2}));
    EXPECT_NE((Velocity{.dx = 1, .dy = 2}), (Velocity{.dx = 1, .dy = 3}));
    EXPECT_NE((Velocity{.dx = 1, .dy = 2}), (Velocity{.dx = 2, .dy = 2}));
}

TEST(VelocityTest, StepByAddsTheVelocityToThePosition)
{
    const auto moved =
        stepBy(GridPosition{.x = 4, .y = 7}, Velocity{.dx = -1, .dy = 2});

    EXPECT_EQ(moved, (GridPosition{.x = 3, .y = 9}));
}

TEST(VelocityTest, StepByWrapsRatherThanOverflowing)
{
    constexpr auto kMax = std::numeric_limits<std::int32_t>::max();
    constexpr auto kMin = std::numeric_limits<std::int32_t>::min();

    const auto moved =
        stepBy(GridPosition{.x = kMax, .y = kMin}, Velocity{.dx = 1, .dy = -1});

    EXPECT_EQ(moved, (GridPosition{.x = kMin, .y = kMax}));
}

TEST(LifetimeTest, DefaultsToNothingLeft)
{
    EXPECT_EQ(Lifetime{}, (Lifetime{.remaining = 0}));
    EXPECT_NE(Lifetime{}, (Lifetime{.remaining = 1}));
}

TEST(TagTest, TwoTagsOfTheSameKindAreEqual)
{
    EXPECT_EQ(Path{}, Path{});
}

TEST(TagTest, TagsOfDifferentKindsAreDifferentTypes)
{
    EXPECT_FALSE((std::is_same_v<Path, Blocked>));
}
