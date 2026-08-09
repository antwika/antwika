#include <gtest/gtest.h>

#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs/World.hpp"
#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    struct Position final
    {
        int x{};
        int y{};

        bool operator==(const Position &) const = default;
    };

    struct Velocity final
    {
        int dx{};

        bool operator==(const Velocity &) const = default;
    };

}

TEST(WorldTest, Create_ReturnsALiveEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();

    EXPECT_TRUE(world.alive(entity));
}

TEST(WorldTest, Add_IsNotVisibleUntilCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Add_IsVisibleAfterCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.commit();

    ASSERT_TRUE(world.has<Position>(entity));
    EXPECT_EQ(world.get<Position>(entity), (Position{1, 2}));
}

TEST(WorldTest, Set_IsNotVisibleUntilTheNextCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.set<Position>(entity, Position{9, 9});

    EXPECT_EQ(world.get<Position>(entity), (Position{1, 2}));

    world.commit();

    EXPECT_EQ(world.get<Position>(entity), (Position{9, 9}));
}

TEST(WorldTest, Set_ThrowsOnAComponentTheEntityLacks)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    EXPECT_THROW(
        world.set<Position>(entity, Position{}), EcsError);
}

TEST(WorldTest, Set_ThrowsOnAnUncommittedAdd)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});

    EXPECT_THROW(world.set<Position>(entity, Position{9, 9}), EcsError);

    world.commit();

    EXPECT_EQ(world.get<Position>(entity), (Position{1, 2}));
}

TEST(WorldTest, Remove_BeatsAnImmediateSetInAPhase)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.set<Position>(entity, Position{9, 9});
    world.remove<Position>(entity);
    world.commit();

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Add_OverwritesAComponentAlreadyHeld)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.add<Position>(entity, Position{3, 4});
    world.commit();

    EXPECT_EQ(world.get<Position>(entity), (Position{3, 4}));
}

TEST(WorldTest, Add_BeatsAnEarlierSetInAPhase)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.set<Position>(entity, Position{9, 9});
    world.add<Position>(entity, Position{3, 4});
    world.commit();

    EXPECT_EQ(world.get<Position>(entity), (Position{3, 4}));
}

TEST(WorldTest, Add_BeatsALaterSetInAPhase)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.add<Position>(entity, Position{3, 4});
    world.set<Position>(entity, Position{9, 9});
    world.commit();

    EXPECT_EQ(world.get<Position>(entity), (Position{3, 4}));
}

TEST(WorldTest, Add_ResolvesTwoInAPhaseAsTheLater)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.add<Position>(entity, Position{3, 4});
    world.commit();

    EXPECT_EQ(world.get<Position>(entity), (Position{3, 4}));
}

TEST(WorldTest, Get_ThrowsOnAMissingComponent)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    EXPECT_THROW(
        static_cast<void>(world.get<Position>(entity)), EcsError);
}

TEST(WorldTest, Remove_LeavesTheComponentGoneAfterCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.remove<Position>(entity);

    EXPECT_TRUE(world.has<Position>(entity));

    world.commit();

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Destroy_LeavesTheEntityAliveUntilCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.destroy(entity);

    EXPECT_TRUE(world.alive(entity));

    world.commit();

    EXPECT_FALSE(world.alive(entity));
    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Get_ThrowsOnADeadEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(
        static_cast<void>(world.get<Position>(entity)), EcsError);
}

TEST(WorldTest, Has_IsFalseRatherThanThrowingWhenDead)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Add_ThrowsOnADeadEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(
        world.add<Position>(entity, Position{}), EcsError);
}

TEST(WorldTest, Destroy_ThrowsOnAnAlreadyDeadEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(world.destroy(entity), EcsError);
}

TEST(WorldTest, Destroy_RetiresAnEntityOncePerPhase)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.destroy(entity);
    EXPECT_NO_THROW(world.destroy(entity));
    EXPECT_NO_THROW(world.commit());

    EXPECT_FALSE(world.alive(entity));
    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Commit_DoesNotReapplyTheFirstCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_NO_THROW(world.commit());
    EXPECT_FALSE(world.alive(entity));
}

TEST(WorldTest, Remove_ThrowsOnADeadEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(world.remove<Position>(entity), EcsError);
}

TEST(WorldTest, Remove_DoesNothingForAComponentNeverHeld)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.commit();

    EXPECT_NO_THROW(world.remove<Position>(entity));
    world.commit();

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Get_ThrowsWhenOnlyAnotherEntityHasIt)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto withPosition = world.create();
    const auto withoutPosition = world.create();
    world.add<Position>(withPosition, Position{1, 2});
    world.commit();

    EXPECT_THROW(
        static_cast<void>(world.get<Position>(withoutPosition)), EcsError);
}

TEST(WorldTest, Set_ThrowsOnADeadEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(world.set<Position>(entity, Position{}), EcsError);
}

TEST(WorldTest, Destroy_LeavesUnrelatedPoolsAlone)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto withPosition = world.create();
    const auto withVelocity = world.create();
    world.add<Position>(withPosition, Position{1, 2});
    world.add<Velocity>(withVelocity, Velocity{3});
    world.commit();

    world.destroy(withPosition);
    world.commit();

    EXPECT_FALSE(world.alive(withPosition));
    ASSERT_TRUE(world.has<Velocity>(withVelocity));
    EXPECT_EQ(world.get<Velocity>(withVelocity), (Velocity{3}));

    world.destroy(withVelocity);
    world.commit();

    EXPECT_FALSE(world.alive(withVelocity));
}

TEST(WorldTest, Destroy_LeavesNoOrphanWhenStagedBeforeAdd)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.destroy(entity);
    world.add<Position>(entity, Position{1, 2});
    world.add<Velocity>(entity, Velocity{3});
    world.commit();

    EXPECT_FALSE(world.alive(entity));
    EXPECT_FALSE(world.has<Position>(entity));
    EXPECT_FALSE(world.has<Velocity>(entity));
    const auto positionView = world.view<Position>();
    EXPECT_EQ(positionView.size(), 0U);
    const auto velocityView = world.view<Velocity>();
    EXPECT_EQ(velocityView.size(), 0U);
}

TEST(WorldTest, View_IsEmptyForAnUnusedComponentType)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto view = world.view<Position>();

    EXPECT_EQ(view.size(), 0U);
}

TEST(WorldTest, View_ReturnsTheEntityForOneComponent)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Velocity>(entity, Velocity{5});
    world.commit();

    const auto view = world.view<Velocity>();
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{entity}));
}

TEST(WorldTest, View_IntersectsMultipleComponentTypes)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto both = world.create();
    const auto positionOnly = world.create();
    world.add<Position>(both, Position{1, 1});
    world.add<Velocity>(both, Velocity{2});
    world.add<Position>(positionOnly, Position{3, 3});
    world.commit();

    const auto view = world.view<Position, Velocity>();
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{both}));

    const auto positionView = world.view<Position>();
    const std::vector<Entity> positionEntities(
        positionView.begin(), positionView.end());

    EXPECT_EQ(positionEntities, (std::vector<Entity>{both, positionOnly}));
}
