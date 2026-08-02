#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/EcsError.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::EcsError;
using antwika::ecs::World;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::Path;
using antwika::game::Walker;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(WalkerTest, DefaultConstructedFacesEastAndIsDueToStep)
{
    constexpr Walker walker;

    EXPECT_EQ(walker.facing, Direction::East);
    EXPECT_EQ(walker.ticksUntilStep, 0U);
}

TEST(WalkerTest, DefaultConstructedHasNowhereItCameFrom)
{
    // A walker that has never stepped is drawn where it stands.
    // Rather than sliding in from a cell it was never on.
    constexpr Walker walker;

    EXPECT_FALSE(walker.from.has_value());
}

TEST(WalkerTest, EqualityComparesTheFacingAndTheCountdown)
{
    constexpr Walker walker{
        .facing = Direction::North, .ticksUntilStep = 1};

    EXPECT_EQ(
        walker,
        (Walker{.facing = Direction::North, .ticksUntilStep = 1}));
    EXPECT_NE(
        walker,
        (Walker{.facing = Direction::South, .ticksUntilStep = 1}));
    EXPECT_NE(
        walker,
        (Walker{.facing = Direction::North, .ticksUntilStep = 0}));
}

TEST(WalkerTest, EqualityComparesWhereItCameFrom)
{
    constexpr Cell origin{.x = 3, .y = 4};

    constexpr Walker walker{
        .facing = Direction::North, .ticksUntilStep = 1, .from = origin};

    EXPECT_EQ(
        walker,
        (Walker{
            .facing = Direction::North,
            .ticksUntilStep = 1,
            .from = origin}));
    EXPECT_NE(
        walker,
        (Walker{
            .facing = Direction::North,
            .ticksUntilStep = 1,
            .from = Cell{.x = 3, .y = 5}}));
    EXPECT_NE(
        walker,
        (Walker{.facing = Direction::North, .ticksUntilStep = 1}));
}

TEST(WalkerTest, PathTagsCompareEqualToEachOther)
{
    // It carries no state, so any two are the same tag.
    EXPECT_EQ(Path{}, Path{});
}

TEST(WalkerTest, WalkerComponentCanBeRemovedFromAnEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Walker>(entity, Walker{});
    world.add<Cell>(entity, Cell{.x = 1, .y = 2});
    world.commit();

    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.has<Walker>(entity));
    EXPECT_FALSE(world.has<Cell>(entity));
}

TEST(WalkerTest, PathComponentCanBeRemovedFromAnEntity)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Path>(entity, Path{});
    world.add<Cell>(entity, Cell{.x = 3, .y = 4});
    world.commit();

    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.has<Path>(entity));
    EXPECT_FALSE(world.has<Cell>(entity));
}

TEST(WalkerTest, SettingAComponentOnADeadEntityIsRefused)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Walker>(entity, Walker{});
    world.commit();
    world.destroy(entity);
    world.commit();

    EXPECT_THROW(world.set<Walker>(entity, Walker{}), EcsError);
    EXPECT_THROW(world.set<Path>(entity, Path{}), EcsError);
    EXPECT_THROW(world.set<Cell>(entity, Cell{}), EcsError);
}

TEST(WalkerTest, SettingAComponentAnEntityNeverHadIsRefused)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Walker>(entity, Walker{});
    world.commit();

    EXPECT_THROW(world.set<Path>(entity, Path{}), EcsError);
    EXPECT_THROW(world.set<Cell>(entity, Cell{}), EcsError);
}

TEST(WalkerTest, EveryComponentCanBeSetAfterItIsAdded)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.add<Walker>(entity, Walker{});
    world.add<Path>(entity, Path{});
    world.add<Cell>(entity, Cell{});
    world.commit();

    world.set<Walker>(entity, Walker{.facing = Direction::South});
    world.set<Path>(entity, Path{});
    world.set<Cell>(entity, Cell{.x = 7, .y = 8});
    world.commit();

    EXPECT_EQ(world.get<Walker>(entity).facing, Direction::South);
    EXPECT_EQ(world.get<Cell>(entity), (Cell{.x = 7, .y = 8}));
    EXPECT_TRUE(world.has<Path>(entity));
}

// The staged add runs after the staged destroy.
// So it must drop the component rather than orphan it in a swept pool.
TEST(WalkerTest, AddingToAnEntityAlreadyStagedForDestructionIsDropped)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto entity = world.create();
    world.destroy(entity);
    world.add<Walker>(entity, Walker{});
    world.add<Path>(entity, Path{});
    world.add<Cell>(entity, Cell{});
    world.commit();

    EXPECT_FALSE(world.alive(entity));
    EXPECT_FALSE(world.has<Walker>(entity));
}

// Destroying sweeps every pool, including ones this entity was never in.
// Three entities with different components.
// Each pool is then asked about one it holds and one it does not.
TEST(WalkerTest, DestroyingSweepsPoolsTheEntityWasNeverIn)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    const auto tile = world.create();
    world.add<Path>(tile, Path{});
    world.add<Cell>(tile, Cell{.x = 1, .y = 1});

    const auto walker = world.create();
    world.add<Walker>(walker, Walker{});
    world.add<Cell>(walker, Cell{.x = 2, .y = 2});

    const auto bare = world.create();
    world.add<Walker>(bare, Walker{});
    world.commit();

    world.destroy(tile);
    world.destroy(walker);
    world.destroy(bare);
    world.commit();

    EXPECT_FALSE(world.alive(tile));
    EXPECT_FALSE(world.alive(walker));
    EXPECT_FALSE(world.alive(bare));
    EXPECT_FALSE(world.has<Path>(tile));
    EXPECT_FALSE(world.has<Walker>(walker));
    EXPECT_FALSE(world.has<Cell>(walker));
}

// The defaulted comparison short-circuits.
// So every field needs a pair that differs in it alone.
TEST(WalkerTest, EqualityComparesEveryFieldIndependently)
{
    const antwika::game::Walker base{
        .facing = antwika::game::Direction::North,
        .kind = antwika::game::WalkerKind::WaterCarrier,
        .carried = 30,
        .stepsUntilHome = 5,
        .home = static_cast<antwika::ecs::Entity>(7),
        .ticksUntilStep = 1,
        .from = Cell{.x = 1, .y = 2}};

    EXPECT_EQ(base, base);

    auto turned = base;
    turned.facing = antwika::game::Direction::South;
    EXPECT_NE(base, turned);

    auto other = base;
    other.kind = antwika::game::WalkerKind::Fireman;
    EXPECT_NE(base, other);

    auto emptied = base;
    emptied.carried = 0;
    EXPECT_NE(base, emptied);

    auto tired = base;
    tired.stepsUntilHome = 0;
    EXPECT_NE(base, tired);

    auto rehomed = base;
    rehomed.home = antwika::ecs::kNullEntity;
    EXPECT_NE(base, rehomed);

    auto later = base;
    later.ticksUntilStep = 0;
    EXPECT_NE(base, later);

    auto elsewhere = base;
    elsewhere.from = Cell{.x = 9, .y = 9};
    EXPECT_NE(base, elsewhere);
}
