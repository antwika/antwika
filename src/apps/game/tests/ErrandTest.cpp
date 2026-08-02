#include "antwika/game/Errand.hpp"

#include <cstddef>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Walker.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;
    using antwika::ecs::World;
    using antwika::game::Cell;
    using antwika::game::Errand;
    using antwika::game::errandLegFromName;
    using antwika::game::errandLegIndex;
    using antwika::game::errandLegName;
    using antwika::game::ErrandLeg;
    using antwika::game::errandTarget;
    using antwika::game::errandTargetOf;
    using antwika::game::kErrandLegCount;
    using antwika::game::Resource;
    using antwika::game::Walker;
    using antwika::game::WalkerKind;
    using antwika::log::mocks::MockLogger;

    class ErrandTest : public ::testing::Test
    {
    protected:
        Entity sendWalker(Walker walker)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, Cell{});
            world.add<Walker>(entity, walker);
            world.commit();
            return entity;
        }

        void give(Entity entity, Errand errand)
        {
            world.add<Errand>(entity, errand);
            world.commit();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
    };
} // namespace

// Every leg names itself, and the names come back as legs.
TEST(ErrandLegTest, ErrandLegName_RoundTripsEveryLeg)
{
    for (std::size_t index = 0; index < kErrandLegCount; ++index)
    {
        const auto leg = static_cast<ErrandLeg>(index);

        EXPECT_EQ(errandLegIndex(leg), index);
        EXPECT_EQ(errandLegFromName(errandLegName(leg)), leg);
    }
}

TEST(ErrandLegTest, ErrandLegFromName_RefusesAName)
{
    EXPECT_FALSE(errandLegFromName("sideways").has_value());
}

TEST(ErrandLegTest, ErrandTarget_IsTheDestinationOnTheWayOut)
{
    const Errand errand{
        .destination = static_cast<Entity>(7),
        .carrying = Resource::Clay,
        .leg = ErrandLeg::Outbound};
    const Walker walker{.home = static_cast<Entity>(3)};

    EXPECT_EQ(errandTarget(errand, walker), static_cast<Entity>(7));
}

TEST(ErrandLegTest, ErrandTarget_IsWhereItCameFromOnTheWayBack)
{
    const Errand errand{
        .destination = static_cast<Entity>(7),
        .carrying = Resource::Clay,
        .leg = ErrandLeg::Returning};
    const Walker walker{.home = static_cast<Entity>(3)};

    EXPECT_EQ(errandTarget(errand, walker), static_cast<Entity>(3));
}

TEST_F(ErrandTest, ErrandTargetOf_IsNobodyForAWalkerWithNoErrand)
{
    const auto walker = sendWalker(Walker{});

    EXPECT_EQ(errandTargetOf(world, walker), kNullEntity);
}

TEST_F(ErrandTest, ErrandTargetOf_IsNobodyForAnErrandBoundNowhere)
{
    const auto walker = sendWalker(Walker{});
    give(walker, Errand{});

    EXPECT_EQ(errandTargetOf(world, walker), kNullEntity);
}

TEST_F(ErrandTest, ErrandTargetOf_IsTheDestinationItWasGiven)
{
    const auto store = world.create();
    const auto walker = sendWalker(Walker{});
    give(walker, Errand{.destination = store});

    EXPECT_EQ(errandTargetOf(world, walker), store);
}

// The pools an errand and a countdown add are swept like any other.
// One entity that has the component, one that never did.
TEST_F(ErrandTest, DestroyingSweepsTheErrandPoolEitherWay)
{
    const auto cart = sendWalker(Walker{});
    give(cart, Errand{});

    const auto plain = sendWalker(Walker{});

    world.destroy(cart);
    world.destroy(plain);
    world.commit();

    EXPECT_FALSE(world.has<Errand>(cart));
    EXPECT_FALSE(world.alive(plain));
}

TEST_F(ErrandTest, AddingAnErrandToADestroyedWalkerIsDropped)
{
    const auto cart = sendWalker(Walker{});

    world.destroy(cart);
    world.add<Errand>(cart, Errand{});
    world.commit();

    EXPECT_FALSE(world.has<Errand>(cart));
}
