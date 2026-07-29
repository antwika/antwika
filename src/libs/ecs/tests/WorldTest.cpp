#include "antwika/ecs/World.hpp"

#include <vector>

#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    struct Position
    {
        int x{};
        int y{};

        bool operator==(const Position &) const = default;
    };

    struct Velocity
    {
        int dx{};

        bool operator==(const Velocity &) const = default;
    };

} // namespace

TEST(WorldTest, CreateReturnsALiveEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();

    EXPECT_TRUE(world.alive(entity));
}

TEST(WorldTest, AddedComponentIsNotVisibleUntilCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, AddedComponentIsVisibleAfterCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.commit();

    ASSERT_TRUE(world.has<Position>(entity));
    EXPECT_EQ(world.get<Position>(entity), (Position{1, 2}));
}

TEST(WorldTest, SetIsNotVisibleUntilTheNextCommit)
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

TEST(WorldTest, SettingAComponentTheEntityDoesNotHaveThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    EXPECT_THROW(
        world.set<Position>(entity, Position{}), EcsError);
}

TEST(WorldTest, GettingAMissingComponentThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    EXPECT_THROW(
        static_cast<void>(world.get<Position>(entity)), EcsError);
}

TEST(WorldTest, RemovedComponentIsGoneAfterCommit)
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

TEST(WorldTest, DestroyedEntityStaysAliveUntilCommit)
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

TEST(WorldTest, GettingAComponentFromADeadEntityThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(
        static_cast<void>(world.get<Position>(entity)), EcsError);
}

TEST(WorldTest, HasOnADeadEntityReturnsFalseRatherThanThrowing)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, AddingAComponentToADeadEntityThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(
        world.add<Position>(entity, Position{}), EcsError);
}

TEST(WorldTest, DestroyingAnAlreadyDeadEntityThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(world.destroy(entity), EcsError);
}

TEST(WorldTest, RemovingAComponentFromADeadEntityThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(world.remove<Position>(entity), EcsError);
}

TEST(WorldTest, RemovingAComponentTheEntityNeverHadIsANoOp)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.commit();

    EXPECT_NO_THROW(world.remove<Position>(entity));
    world.commit();

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, GettingAComponentAnotherEntityHasButThisOneLacksThrows)
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

TEST(WorldTest, SettingAComponentOnADeadEntityThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(world.set<Position>(entity, Position{}), EcsError);
}

TEST(WorldTest, DestroyingAnEntityLeavesUnrelatedPoolsForOthersAlone)
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

TEST(WorldTest, DestroyStagedBeforeAddInTheSameCommitLeavesNoOrphan)
{
    // Covers two distinct T instantiations of the deferred re-check.
    // Each is compiled and branch-tracked separately.
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

TEST(WorldTest, ViewOverAnUnusedComponentTypeIsEmpty)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto view = world.view<Position>();

    EXPECT_EQ(view.size(), 0U);
}

TEST(WorldTest, ViewOverASingleComponentTypeWithDataReturnsThatEntity)
{
    // Position's view<T>() already gets exercised with real data.
    // Velocity's never did until now.
    // It was only ever empty, or paired with Position.
    // That pairing is a separate instantiation from Velocity alone.
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Velocity>(entity, Velocity{5});
    world.commit();

    const auto view = world.view<Velocity>();
    const std::vector<Entity> entities(view.begin(), view.end());

    EXPECT_EQ(entities, (std::vector<Entity>{entity}));
}

TEST(WorldTest, ViewIntersectsMultipleComponentTypes)
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
