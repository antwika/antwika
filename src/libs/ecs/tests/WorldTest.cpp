#include <gtest/gtest.h>

#include <cstddef>
#include <utility>
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

    template <std::size_t At>
    struct Padding final
    {
        int value{};
    };

    template <std::size_t... Ats>
    void addFillers(
        World &world, Entity entity, std::index_sequence<Ats...>)
    {
        (world.add<Padding<Ats>>(entity, Padding<Ats>{}), ...);
    }

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
    for (int index = 0; index < 6; ++index)
    {
        const auto entity = world.create();
        world.add<Position>(entity, Position{index, index});
        entities.push_back(entity);
    }
    world.commit();

    world.destroy(entities[1]);
    world.destroy(entities[4]);
    world.commit();

    const auto view = world.view<Position>();
    const std::vector<Entity> orderEntities(view.begin(), view.end());

    EXPECT_EQ(
        orderEntities,
        (std::vector<Entity>{
            entities[0], entities[2], entities[3], entities[5]}));
}

TEST(WorldTest, Destroy_ClearsEveryPoolOfABatchInOneCommit)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    std::vector<Entity> entities;
    for (int index = 0; index < 4; ++index)
    {
        const auto entity = world.create();
        world.add<Position>(entity, Position{index, index});
        world.add<Velocity>(entity, Velocity{index});
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

    World firstWorld(logger);
    const auto entity = firstWorld.create();
    firstWorld.add<Unregistered>(entity, Unregistered{1});
    firstWorld.add<Registered>(entity, Registered{2});
    firstWorld.commit();

    World secondWorld(logger);
    const auto secondEntity = secondWorld.create();
    secondWorld.add<Registered>(secondEntity, Registered{3});
    secondWorld.commit();

    EXPECT_FALSE(secondWorld.has<Unregistered>(secondEntity));
    EXPECT_EQ(secondWorld.view<Unregistered>().size(), 0U);
}

TEST(WorldTest, Add_FillsAPoolAnotherWorldOpenedFirst)
{
    NiceMock<MockLogger> logger;

    World firstWorld(logger);
    const auto entity = firstWorld.create();
    firstWorld.add<Unregistered>(entity, Unregistered{1});
    firstWorld.add<Registered>(entity, Registered{2});
    firstWorld.commit();

    World secondWorld(logger);
    const auto secondEntity = secondWorld.create();
    secondWorld.add<Registered>(secondEntity, Registered{3});
    secondWorld.commit();

    secondWorld.add<Unregistered>(secondEntity, Unregistered{4});
    secondWorld.commit();

    ASSERT_TRUE(secondWorld.has<Unregistered>(secondEntity));
    EXPECT_EQ(secondWorld.get<Unregistered>(secondEntity), (Unregistered{4}));
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
    const auto doomedEntity = world.create();
    const auto keptEntity = world.create();

    world.destroy(doomedEntity);
    world.add<Position>(doomedEntity, Position{1, 2});
    world.add<Position>(keptEntity, Position{3, 4});
    world.commit();

    EXPECT_FALSE(world.alive(doomedEntity));
    EXPECT_FALSE(world.has<Position>(doomedEntity));
    ASSERT_TRUE(world.has<Position>(keptEntity));
    EXPECT_EQ(world.get<Position>(keptEntity), (Position{3, 4}));
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
    const std::vector<Entity> listedEntities(view.begin(), view.end());
    EXPECT_EQ(listedEntities, (std::vector<Entity>{entity}));

    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.has<Unregistered>(entity));
    EXPECT_FALSE(world.has<Registered>(entity));
}

TEST(WorldTest, Add_RefusesMoreComponentTypesThanAWorldHolds)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    addFillers(
        world,
        entity,
        std::make_index_sequence<World::kMaxComponents>{});

    EXPECT_THROW(
        (world.add<Padding<World::kMaxComponents>>(
            entity, Padding<World::kMaxComponents>{})),
        EcsError);
}

TEST(WorldTest, Claim_GivesNoEntityAComponent)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.claim<Position>();

    EXPECT_FALSE(world.has<Position>(entity));
}

TEST(WorldTest, Claim_LeavesATypeAlreadyHeldAsItStands)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.commit();
    world.claim<Position>();

    ASSERT_TRUE(world.has<Position>(entity));
    EXPECT_EQ(world.get<Position>(entity), (Position{1, 2}));
}

TEST(WorldTest, Claim_TakesUpOneOfTheSlotsAWorldHolds)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.claim<Position>();
    world.add<Position>(entity, Position{3, 4});
    world.commit();

    ASSERT_TRUE(world.has<Position>(entity));
    EXPECT_EQ(world.get<Position>(entity), (Position{3, 4}));
}

TEST(WorldTest, ForgetComponents_LeavesEveryEntityStanding)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.commit();
    world.forgetComponents();

    EXPECT_TRUE(world.alive(entity));
}

TEST(WorldTest, ForgetComponents_TakesEveryComponentAway)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.add<Velocity>(entity, Velocity{3});
    world.commit();
    world.forgetComponents();

    EXPECT_FALSE(world.has<Position>(entity));
    EXPECT_FALSE(world.has<Velocity>(entity));
}

TEST(WorldTest, ForgetComponents_LetsTheSameTypeBeTakenUpAfresh)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.commit();
    world.forgetComponents();
    world.add<Position>(entity, Position{4, 5});
    world.commit();

    ASSERT_TRUE(world.has<Position>(entity));
    EXPECT_EQ(world.get<Position>(entity), (Position{4, 5}));
}

TEST(WorldTest, ForgetComponents_DropsWhatWasStagedAndNotYetCommitted)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    world.add<Position>(entity, Position{1, 2});
    world.forgetComponents();
    world.commit();

    EXPECT_FALSE(world.has<Position>(entity));
}

