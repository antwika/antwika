#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/FireCall.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WalkerSystem.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::Errand;
using antwika::game::ErrandLeg;
using antwika::game::Journey;
using antwika::game::PathIndex;
using antwika::game::Walker;
using antwika::game::WalkerSystem;
using antwika::log::mocks::MockLogger;

namespace
{
    constexpr antwika::game::GridExtent kExtent{
        .width = 16, .height = 16};

    class WalkerSystemTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] Entity addWalker(Cell at, Direction facing)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(entity, Walker{.facing = facing});
            world.commit();
            return entity;
        }

        void layPath(const std::vector<Cell> &cells)
        {
            for (const auto cell : cells)
            {
                paths.insert(cell);
            }
        }

        void tick()
        {
            system.update(world, 0);
            world.commit();
        }

        void stride()
        {
            for (std::uint8_t held = 0;
                 held < antwika::game::kTicksPerStep;
                 ++held)
            {
                tick();
            }
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        antwika::game::BuildingIndex built;
        WalkerSystem system{paths, built, kExtent};
    };
}

TEST_F(WalkerSystemTest, Update_AdvancesOneCellAlongAStraightPath)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::East);
}

TEST_F(WalkerSystemTest, Update_StaysPutOnTheTickAfterAStep)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();
    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::East);
}

TEST_F(WalkerSystemTest, Update_AdvancesAgainOnceItsWaitHasPassed)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    stride();
    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_KeepsEachWalkersOwnCadence)
{
    layPath(
        {{.x = 0, .y = 0},
         {.x = 1, .y = 0},
         {.x = 2, .y = 0},
         {.x = 3, .y = 0},
         {.x = 4, .y = 0}});
    const auto early = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();

    const auto late = addWalker(Cell{.x = 3, .y = 0}, Direction::East);

    stride();

    EXPECT_EQ(world.get<Cell>(early), (Cell{.x = 2, .y = 0}));
    EXPECT_EQ(world.get<Cell>(late), (Cell{.x = 4, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_TakesTheRightArmAtATJunction)
{
    layPath(
        {{.x = 0, .y = 0},
         {.x = 1, .y = 0},
         {.x = 2, .y = 0},
         {.x = 1, .y = 1}});
    const auto walker = addWalker(Cell{.x = 1, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 1, .y = 1}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::South);
}

TEST_F(WalkerSystemTest, Update_ComesBackFromADeadEnd)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    const auto walker = addWalker(Cell{.x = 1, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 0, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::West);
}

TEST_F(WalkerSystemTest, Update_LeavesAWalkerOnAnIsolatedTileWhereItIs)
{
    layPath({{.x = 4, .y = 4}});
    const auto walker = addWalker(Cell{.x = 4, .y = 4}, Direction::North);

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 4, .y = 4}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::North);
}

TEST_F(WalkerSystemTest, Update_MovesEveryWalkerNotJustTheFirst)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto first = addWalker(Cell{.x = 0, .y = 0}, Direction::East);
    const auto second = addWalker(Cell{.x = 1, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Cell>(first), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Cell>(second), (Cell{.x = 2, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_LetsTwoWalkersPassThroughEachOther)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    const auto eastbound = addWalker(Cell{.x = 0, .y = 0}, Direction::East);
    const auto westbound = addWalker(Cell{.x = 1, .y = 0}, Direction::West);

    tick();

    EXPECT_EQ(world.get<Cell>(eastbound), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Cell>(westbound), (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_LeavesAPathTileAlone)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});
    const auto tile = world.create();
    world.add<Cell>(tile, Cell{.x = 0, .y = 0});
    world.commit();

    tick();

    EXPECT_EQ(world.get<Cell>(tile), (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_DoesNothingWithNoWalkersAtAll)
{
    layPath({{.x = 0, .y = 0}});

    EXPECT_NO_THROW(tick());
}

TEST_F(WalkerSystemTest, Update_RecordsTheCellAStepBeganFrom)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(world.get<Walker>(walker).from, (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_KeepsTheStartCellWhileCountingDown)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();
    tick();

    EXPECT_EQ(
        world.get<Walker>(walker).ticksUntilStep,
        antwika::game::kTicksPerStep - 2U);
    EXPECT_EQ(world.get<Walker>(walker).from, (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_MovesTheStartCellOnEveryStep)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    stride();
    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).from, (Cell{.x = 1, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_LeavesNoStartCellForAWalkerThatCannotMove)
{
    layPath({{.x = 5, .y = 5}});
    const auto walker = addWalker(Cell{.x = 5, .y = 5}, Direction::East);

    tick();

    EXPECT_FALSE(world.get<Walker>(walker).from.has_value());
}

TEST_F(WalkerSystemTest, Update_TiresAWalkerOneStepAtATime)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});
    const auto walker = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    tick();

    EXPECT_EQ(
        world.get<Walker>(walker).stepsUntilHome,
        antwika::game::kRoamingSteps - 1);
}

TEST_F(WalkerSystemTest, Update_DoesNotTireAWalkerThatCannotMove)
{
    layPath({{.x = 5, .y = 5}});
    const auto walker = addWalker(Cell{.x = 5, .y = 5}, Direction::East);

    tick();

    EXPECT_EQ(
        world.get<Walker>(walker).stepsUntilHome,
        antwika::game::kRoamingSteps);
}

TEST_F(WalkerSystemTest, Update_RemovesATiredWalkerThatNobodySent)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 0, .y = 0});
    world.add<Walker>(walker, Walker{.stepsUntilHome = 0});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(walker));
}

TEST_F(WalkerSystemTest, Update_WalksATiredWalkerBackTowardsItsBuilding)
{
    layPath({{.x = 1, .y = 0}, {.x = 2, .y = 0}, {.x = 3, .y = 0}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 3, .y = 0});
    world.add<Walker>(
        walker,
        Walker{
            .facing = Direction::East,
            .stepsUntilHome = 0,
            .home = home});
    world.commit();

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
    EXPECT_EQ(world.get<Walker>(walker).facing, Direction::West);
}

TEST_F(WalkerSystemTest, Update_RemovesAWalkerAsItReachesItsBuilding)
{
    layPath({{.x = 1, .y = 0}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 1, .y = 0});
    world.add<Walker>(walker, Walker{.stepsUntilHome = 0, .home = home});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(walker));
}

TEST_F(WalkerSystemTest, Update_RemovesATiredWalkerWhoseBuildingIsGone)
{
    layPath({{.x = 1, .y = 0}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 1, .y = 0});
    world.add<Walker>(
        walker, Walker{.stepsUntilHome = 0, .home = home});
    world.commit();

    world.destroy(home);
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(walker));
}

TEST_F(WalkerSystemTest, Update_RemovesATiredWalkerWalledOffFromItsHome)
{
    layPath({{.x = 5, .y = 5}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 5, .y = 5});
    world.add<Walker>(
        walker, Walker{.stepsUntilHome = 0, .home = home});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(walker));
}

TEST_F(WalkerSystemTest, Update_KeepsATiredWalkerCountingDownBetweenSteps)
{
    layPath({{.x = 1, .y = 0}, {.x = 2, .y = 0}, {.x = 3, .y = 0}});

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 0, .y = 0});
    world.add<antwika::game::Building>(home, antwika::game::Building{});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 3, .y = 0});
    world.add<Walker>(
        walker, Walker{.stepsUntilHome = 0, .home = home});
    world.commit();

    tick();
    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 2, .y = 0}));
    EXPECT_TRUE(world.alive(walker));
}

namespace
{
    TEST_F(WalkerSystemTest, Update_ArrivesAtAnyCellOfItsBuildingsBlock)
    {
        layPath({{.x = 4, .y = 1}});

        const auto home = world.create();
        world.add<Cell>(home, Cell{.x = 2, .y = 1});
        world.add<antwika::game::Building>(
            home,
            antwika::game::Building{
                .kind = antwika::game::BuildingKind::Farm});

        const auto walker = world.create();
        world.add<Cell>(walker, Cell{.x = 4, .y = 1});
        world.add<Walker>(
            walker, Walker{.stepsUntilHome = 0, .home = home});
        world.commit();

        tick();

        EXPECT_FALSE(world.alive(walker));
    }
}

namespace
{
    struct Bound final
    {
        Entity building;
        Entity walker;
    };
}

namespace
{
    Bound sendOnErrand(
        World &world,
        Cell from,
        Cell to,
        BuildingKind kind,
        ErrandLeg leg)
    {
        const auto building = world.create();
        world.add<Cell>(building, to);
        world.add<Building>(building, Building{.kind = kind});

        const auto walker = world.create();
        world.add<Cell>(walker, from);
        world.add<Walker>(
            walker,
            Walker{
                .kind = antwika::game::WalkerKind::CartPusher,
                .home = building});
        world.add<Errand>(
            walker,
            Errand{
                .destination =
                    leg == ErrandLeg::Outbound ? building
                                               : antwika::ecs::kNullEntity,
                .leg = leg});
        world.commit();
        return Bound{.building = building, .walker = walker};
    }
}

TEST_F(WalkerSystemTest, Update_StepsAnErrandWalkerTowardsItsDestination)
{
    layPath(
        {{.x = 0, .y = 0},
         {.x = 1, .y = 0},
         {.x = 2, .y = 0},
         {.x = 0, .y = 1}});

    const auto errand = sendOnErrand(
        world,
        Cell{.x = 0, .y = 0},
        Cell{.x = 3, .y = 0},
        BuildingKind::Storage,
        ErrandLeg::Outbound);

    tick();

    EXPECT_EQ(world.get<Cell>(errand.walker), (Cell{.x = 1, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_HoldsAnOutboundWalkerAtItsDestination)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});

    const auto errand = sendOnErrand(
        world,
        Cell{.x = 1, .y = 0},
        Cell{.x = 2, .y = 0},
        BuildingKind::Storage,
        ErrandLeg::Outbound);

    tick();

    EXPECT_TRUE(world.alive(errand.walker));
    EXPECT_EQ(world.get<Cell>(errand.walker), (Cell{.x = 1, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_RemovesAReturningWalkerThatGotHome)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});

    const auto errand = sendOnErrand(
        world,
        Cell{.x = 1, .y = 0},
        Cell{.x = 2, .y = 0},
        BuildingKind::Farm,
        ErrandLeg::Returning);

    tick();

    EXPECT_FALSE(world.alive(errand.walker));
}

TEST_F(WalkerSystemTest, Update_RemovesAnErrandWalkerWhoseStoreIsGone)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});

    const auto errand = sendOnErrand(
        world,
        Cell{.x = 0, .y = 0},
        Cell{.x = 3, .y = 0},
        BuildingKind::Storage,
        ErrandLeg::Outbound);

    world.destroy(errand.building);
    world.commit();

    EXPECT_NO_THROW(tick());
    EXPECT_FALSE(world.alive(errand.walker));
}

TEST_F(WalkerSystemTest, Update_RemovesAnErrandWalkerWithNoRouteLeft)
{
    layPath({{.x = 0, .y = 0}});

    const auto errand = sendOnErrand(
        world,
        Cell{.x = 0, .y = 0},
        Cell{.x = 5, .y = 5},
        BuildingKind::Storage,
        ErrandLeg::Outbound);

    tick();

    EXPECT_FALSE(world.alive(errand.walker));
}

TEST_F(WalkerSystemTest, Update_RoamsAWalkerWhoseErrandNamesNowhere)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});

    const auto walker = world.create();
    world.add<Cell>(walker, Cell{.x = 0, .y = 0});
    world.add<Walker>(walker, Walker{.facing = Direction::East});
    world.add<Errand>(walker, Errand{});
    world.commit();

    tick();

    EXPECT_EQ(world.get<Cell>(walker), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(
        world.get<Walker>(walker).stepsUntilHome,
        antwika::game::kRoamingSteps - 1);
}

TEST_F(WalkerSystemTest, Update_WalksAMigrantTowardsTheHouseItNames)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 0}});

    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 2, .y = 1});
    world.add<Building>(house, Building{.kind = BuildingKind::House});
    world.commit();

    const auto mover = addWalker(Cell{.x = 0, .y = 0}, Direction::East);
    world.add<Journey>(
        mover, Journey{.towards = Cell{.x = 2, .y = 1}, .house = house});
    world.commit();

    tick();

    EXPECT_EQ(world.get<Cell>(mover), (Cell{.x = 1, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_StandsAMigrantAtTheDoorItReached)
{
    layPath({{.x = 1, .y = 0}, {.x = 2, .y = 0}});

    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 2, .y = 1});
    world.add<Building>(house, Building{.kind = BuildingKind::House});
    world.commit();

    const auto mover = addWalker(Cell{.x = 2, .y = 0}, Direction::East);
    world.add<Journey>(
        mover, Journey{.towards = Cell{.x = 2, .y = 1}, .house = house});
    world.commit();

    tick();

    EXPECT_TRUE(world.alive(mover));
    EXPECT_EQ(world.get<Cell>(mover), (Cell{.x = 2, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_RetiresAMigrantThatReachedTheWayOut)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});

    const auto leaver = addWalker(Cell{.x = 1, .y = 0}, Direction::West);
    world.add<Journey>(leaver, Journey{.towards = Cell{.x = 0, .y = 0}});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(leaver));
}

TEST_F(WalkerSystemTest, Update_RetiresAMigrantWhoseHouseHasGone)
{
    layPath({{.x = 0, .y = 0}, {.x = 1, .y = 0}});

    const auto mover = addWalker(Cell{.x = 0, .y = 0}, Direction::East);
    world.add<Journey>(
        mover,
        Journey{
            .towards = Cell{.x = 2, .y = 1},
            .house = static_cast<Entity>(99)});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(mover));
}

TEST_F(WalkerSystemTest, Update_RetiresAMigrantWithNoRouteLeft)
{
    const auto leaver = addWalker(Cell{.x = 1, .y = 1}, Direction::East);
    world.add<Journey>(leaver, Journey{.towards = Cell{.x = 9, .y = 9}});
    world.commit();

    for (const auto around : {
             Cell{.x = 0, .y = 1},
             Cell{.x = 2, .y = 1},
             Cell{.x = 1, .y = 0},
             Cell{.x = 1, .y = 2}})
    {
        (void)built.insert(
            around, antwika::game::footprintOf(BuildingKind::Well));
    }

    tick();

    EXPECT_FALSE(world.alive(leaver));
}

TEST_F(WalkerSystemTest, OperatorEquals_ComparesEveryJourneyField)
{
    const Journey base{
        .towards = Cell{.x = 1, .y = 2},
        .house = static_cast<Entity>(3)};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto elsewhere = base;
    elsewhere.towards = Cell{.x = 2, .y = 1};
    EXPECT_NE(base, elsewhere);

    auto leaving = base;
    leaving.house = antwika::ecs::kNullEntity;
    EXPECT_NE(base, leaving);
}

TEST_F(WalkerSystemTest, Update_KeepsNoJourneyOnAWalkerAlreadyRetired)
{
    const auto leaver = addWalker(Cell{.x = 0, .y = 0}, Direction::East);

    world.destroy(leaver);
    world.add<Journey>(leaver, Journey{.towards = Cell{.x = 1, .y = 1}});
    world.commit();

    EXPECT_FALSE(world.alive(leaver));
    EXPECT_FALSE(world.has<Journey>(leaver));
}

TEST_F(WalkerSystemTest, Update_WalksAFiremanAcrossOpenGroundToAFire)
{
    const auto fire = world.create();
    world.add<Cell>(fire, Cell{.x = 3, .y = 0});
    world.add<antwika::game::Ruin>(
        fire, antwika::game::Ruin{.kind = BuildingKind::House});
    (void)built.insert(
        Cell{.x = 3, .y = 0},
        antwika::game::footprintOf(BuildingKind::House));

    const auto fireman = addWalker(Cell{.x = 0, .y = 0}, Direction::West);
    world.add<antwika::game::FireCall>(
        fireman, antwika::game::FireCall{.target = fire});
    world.commit();

    tick();

    EXPECT_EQ(world.get<Cell>(fireman), (Cell{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<Walker>(fireman).facing, Direction::East);
    EXPECT_EQ(
        world.get<Walker>(fireman).from, (Cell{.x = 0, .y = 0}));
}

TEST_F(WalkerSystemTest, Update_PutsTheFireOutAndTheFiremanWithIt)
{
    const auto fire = world.create();
    world.add<Cell>(fire, Cell{.x = 2, .y = 0});
    world.add<antwika::game::Ruin>(
        fire, antwika::game::Ruin{.kind = BuildingKind::House});
    (void)built.insert(
        Cell{.x = 2, .y = 0},
        antwika::game::footprintOf(BuildingKind::House));

    const auto fireman = addWalker(Cell{.x = 1, .y = 0}, Direction::East);
    world.add<antwika::game::FireCall>(
        fireman, antwika::game::FireCall{.target = fire});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(fireman));
    EXPECT_EQ(
        world.get<antwika::game::Ruin>(fire).state,
        antwika::game::RuinState::Debris);
    EXPECT_EQ(world.get<antwika::game::Ruin>(fire).ticksUntilOut, 0);
}

TEST_F(WalkerSystemTest, Update_RetiresAFiremanWhoseFireBurntOutFirst)
{
    const auto debris = world.create();
    world.add<Cell>(debris, Cell{.x = 3, .y = 0});
    world.add<antwika::game::Ruin>(
        debris,
        antwika::game::Ruin{
            .kind = BuildingKind::House,
            .state = antwika::game::RuinState::Debris,
            .ticksUntilOut = 0});

    const auto fireman = addWalker(Cell{.x = 0, .y = 0}, Direction::East);
    world.add<antwika::game::FireCall>(
        fireman, antwika::game::FireCall{.target = debris});
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(fireman));
}

TEST_F(WalkerSystemTest, Update_RetiresAFiremanWhoseFireWasRazed)
{
    const auto fire = world.create();
    world.add<Cell>(fire, Cell{.x = 3, .y = 0});
    world.add<antwika::game::Ruin>(
        fire, antwika::game::Ruin{.kind = BuildingKind::House});

    const auto fireman = addWalker(Cell{.x = 0, .y = 0}, Direction::East);
    world.add<antwika::game::FireCall>(
        fireman, antwika::game::FireCall{.target = fire});
    world.destroy(fire);
    world.commit();

    tick();

    EXPECT_FALSE(world.alive(fireman));
}

TEST_F(WalkerSystemTest, Update_RetiresAFiremanWalledOffFromTheFire)
{
    const auto fire = world.create();
    world.add<Cell>(fire, Cell{.x = 9, .y = 9});
    world.add<antwika::game::Ruin>(
        fire, antwika::game::Ruin{.kind = BuildingKind::House});
    (void)built.insert(
        Cell{.x = 9, .y = 9},
        antwika::game::footprintOf(BuildingKind::House));

    const auto fireman = addWalker(Cell{.x = 1, .y = 1}, Direction::East);
    world.add<antwika::game::FireCall>(
        fireman, antwika::game::FireCall{.target = fire});
    world.commit();

    for (const auto around : {
             Cell{.x = 0, .y = 1},
             Cell{.x = 2, .y = 1},
             Cell{.x = 1, .y = 0},
             Cell{.x = 1, .y = 2}})
    {
        (void)built.insert(
            around, antwika::game::footprintOf(BuildingKind::Well));
    }

    tick();

    EXPECT_FALSE(world.alive(fireman));
}
