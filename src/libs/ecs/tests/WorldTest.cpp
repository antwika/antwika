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

TEST(WorldTest, Destroy_KeepsTheOrderOfTheSurvivorsInAView)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    std::vector<Entity> entities;
    for (int at = 0; at < 6; ++at)
    {
        const auto entity = world.create();
        world.add<Position>(entity, Position{at, at});
        entities.push_back(entity);
    }
    world.commit();

    world.destroy(entities[1]);
    world.destroy(entities[4]);
    world.commit();

    const auto view = world.view<Position>();
    const std::vector<Entity> order(view.begin(), view.end());

    EXPECT_EQ(
        order,
        (std::vector<Entity>{
            entities[0], entities[2], entities[3], entities[5]}));
}

TEST(WorldTest, Destroy_ClearsEveryPoolOfABatchInOneCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    std::vector<Entity> entities;
    for (int at = 0; at < 4; ++at)
    {
        const auto entity = world.create();
        world.add<Position>(entity, Position{at, at});
        world.add<Velocity>(entity, Velocity{at});
        entities.push_back(entity);
    }
    world.commit();

    for (const auto entity : entities)
    {
        world.destroy(entity);
    }
    world.commit();

    for (const auto entity : entities)
    {
        EXPECT_FALSE(world.alive(entity));
        EXPECT_FALSE(world.has<Position>(entity));
        EXPECT_FALSE(world.has<Velocity>(entity));
    }

    EXPECT_EQ(world.view<Position>().size(), 0U);
    EXPECT_EQ(world.view<Velocity>().size(), 0U);
}

namespace
{

    struct Registered final
    {
        int value{};

        bool operator==(const Registered &) const = default;
    };

    struct Unregistered final
    {
        int value{};

        bool operator==(const Unregistered &) const = default;
    };

}

TEST(WorldTest, Has_IsFalseForATypeAnotherWorldRegisteredFirst)
{
    NiceMock<MockLogger> logger;

    World first(logger);
    const auto entity = first.create();
    first.add<Unregistered>(entity, Unregistered{1});
    first.add<Registered>(entity, Registered{2});
    first.commit();

    World second(logger);
    const auto other = second.create();
    second.add<Registered>(other, Registered{3});
    second.commit();

    EXPECT_FALSE(second.has<Unregistered>(other));
    EXPECT_EQ(second.view<Unregistered>().size(), 0U);
}

TEST(WorldTest, Add_FillsAPoolAnotherWorldOpenedFirst)
{
    NiceMock<MockLogger> logger;

    World first(logger);
    const auto entity = first.create();
    first.add<Unregistered>(entity, Unregistered{1});
    first.add<Registered>(entity, Registered{2});
    first.commit();

    World second(logger);
    const auto other = second.create();
    second.add<Registered>(other, Registered{3});
    second.commit();

    second.add<Unregistered>(other, Unregistered{4});
    second.commit();

    ASSERT_TRUE(second.has<Unregistered>(other));
    EXPECT_EQ(second.get<Unregistered>(other), (Unregistered{4}));
}

TEST(WorldTest, Remove_BeatsAnEarlierAddInAPhase)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.remove<Position>(entity);
    world.commit();

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Add_BeatsAnEarlierRemoveInAPhase)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{1, 2});
    world.commit();

    world.remove<Position>(entity);
    world.add<Position>(entity, Position{7, 8});
    world.commit();

    ASSERT_TRUE(world.has<Position>(entity));
    EXPECT_EQ(world.get<Position>(entity), (Position{7, 8}));
}

TEST(WorldTest, Commit_LeavesADrainedBufferAlone)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.commit();
    world.commit();

    ASSERT_TRUE(world.has<Position>(entity));
    EXPECT_EQ(world.get<Position>(entity), (Position{1, 2}));
}

TEST(WorldTest, Add_IsDiscardedWhenTheEntityDiesInTheSamePhase)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto doomed = world.create();
    const auto kept = world.create();

    world.destroy(doomed);
    world.add<Position>(doomed, Position{1, 2});
    world.add<Position>(kept, Position{3, 4});
    world.commit();

    EXPECT_FALSE(world.alive(doomed));
    EXPECT_FALSE(world.has<Position>(doomed));
    ASSERT_TRUE(world.has<Position>(kept));
    EXPECT_EQ(world.get<Position>(kept), (Position{3, 4}));
}

TEST(WorldTest, Destroy_ClearsBothPoolsOfATypePairSharedAcrossWorlds)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Unregistered>(entity, Unregistered{1});
    world.add<Registered>(entity, Registered{2});
    world.commit();

    ASSERT_TRUE(world.has<Unregistered>(entity));
    ASSERT_TRUE(world.has<Registered>(entity));

    const auto view = world.view<Unregistered>();
    const std::vector<Entity> listed(view.begin(), view.end());
    EXPECT_EQ(listed, (std::vector<Entity>{entity}));

    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.has<Unregistered>(entity));
    EXPECT_FALSE(world.has<Registered>(entity));
}
