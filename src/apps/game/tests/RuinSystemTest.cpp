#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/RuinSystem.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/FireCall.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::BuildingKind;
using antwika::game::Cell;
using antwika::game::FireCall;
using antwika::game::kBurnDurationTicks;
using antwika::game::Ruin;
using antwika::game::RuinState;
using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::footprintOf;
using antwika::game::GameConfig;
using antwika::game::RuinSystem;
using antwika::game::Walker;
using antwika::game::WalkerKind;
using antwika::log::mocks::MockLogger;

namespace
{
    constexpr antwika::game::GridExtent kExtent{
        .width = 16, .height = 16};

    class RuinSystemTest : public ::testing::Test
    {
    protected:
        Entity smoulder(Cell at, Ruin ruin)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Ruin>(entity, ruin);
            world.commit();
            return entity;
        }

        Entity walkerOf(Cell at, WalkerKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(entity, Walker{.kind = kind});
            world.commit();
            return entity;
        }

        void run(std::size_t ticks)
        {
            for (std::size_t tick = 0; tick < ticks; ++tick)
            {
                system.update(world, tick);
                world.commit();
            }
        }

        Entity standing(Cell at, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            (void)built.insert(at, footprintOf(kind));
            world.commit();
            return entity;
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        antwika::game::BuildingIndex built;
        RuinSystem system{built, kExtent, GameConfig{}};
    };
}

TEST_F(RuinSystemTest, Update_BurnsAFireDownOneTickAtATime)
{
    const auto fire = smoulder(
        Cell{.x = 2, .y = 2}, Ruin{.kind = BuildingKind::House});

    run(1);

    EXPECT_EQ(world.get<Ruin>(fire).state, RuinState::Burning);
    EXPECT_EQ(
        world.get<Ruin>(fire).ticksUntilOut, kBurnDurationTicks - 1);
}

TEST_F(RuinSystemTest, Update_TurnsABurntOutFireToDebris)
{
    const auto fire = smoulder(
        Cell{.x = 2, .y = 2},
        Ruin{.kind = BuildingKind::House, .ticksUntilOut = 1});

    run(2);

    EXPECT_EQ(world.get<Ruin>(fire).state, RuinState::Debris);
}

TEST_F(RuinSystemTest, Update_LeavesDebrisExactlyAsItIs)
{
    const auto debris = smoulder(
        Cell{.x = 2, .y = 2},
        Ruin{
            .kind = BuildingKind::House,
            .state = RuinState::Debris,
            .ticksUntilOut = 0});

    run(3);

    EXPECT_EQ(world.get<Ruin>(debris).state, RuinState::Debris);
    EXPECT_EQ(world.get<Ruin>(debris).ticksUntilOut, 0);
}

TEST_F(RuinSystemTest, Update_TasksTheNearestFireman)
{
    const auto fire = smoulder(
        Cell{.x = 2, .y = 2}, Ruin{.kind = BuildingKind::House});
    walkerOf(Cell{.x = 9, .y = 9}, WalkerKind::Fireman);
    const auto near = walkerOf(Cell{.x = 4, .y = 2}, WalkerKind::Fireman);

    run(1);

    ASSERT_TRUE(world.has<FireCall>(near));
    EXPECT_EQ(world.get<FireCall>(near).target, fire);
}

TEST_F(RuinSystemTest, Update_BreaksADistanceTieByAscendingCell)
{
    const auto fire = smoulder(
        Cell{.x = 2, .y = 2}, Ruin{.kind = BuildingKind::House});

    walkerOf(Cell{.x = 3, .y = 3}, WalkerKind::Fireman);
    const auto lower =
        walkerOf(Cell{.x = 1, .y = 1}, WalkerKind::Fireman);

    run(1);

    ASSERT_TRUE(world.has<FireCall>(lower));
    EXPECT_EQ(world.get<FireCall>(lower).target, fire);
}

TEST_F(RuinSystemTest, Update_BreaksASharedCellTieByAscendingEntity)
{
    const auto fire = smoulder(
        Cell{.x = 2, .y = 2}, Ruin{.kind = BuildingKind::House});

    const auto first =
        walkerOf(Cell{.x = 4, .y = 2}, WalkerKind::Fireman);
    const auto second =
        walkerOf(Cell{.x = 4, .y = 2}, WalkerKind::Fireman);

    run(1);

    ASSERT_TRUE(world.has<FireCall>(first));
    EXPECT_EQ(world.get<FireCall>(first).target, fire);
    EXPECT_FALSE(world.has<FireCall>(second));
}

TEST_F(RuinSystemTest, Update_PassesOverEveryOtherKindOfWalker)
{
    smoulder(Cell{.x = 2, .y = 2}, Ruin{.kind = BuildingKind::House});
    const auto carrier =
        walkerOf(Cell{.x = 3, .y = 2}, WalkerKind::WaterCarrier);

    run(1);

    EXPECT_FALSE(world.has<FireCall>(carrier));
}

TEST_F(RuinSystemTest, Update_SendsNobodyWhenNobodyIsFree)
{
    const auto first = smoulder(
        Cell{.x = 2, .y = 2}, Ruin{.kind = BuildingKind::House});
    const auto fireman =
        walkerOf(Cell{.x = 4, .y = 2}, WalkerKind::Fireman);

    run(1);

    ASSERT_TRUE(world.has<FireCall>(fireman));

    const auto second = smoulder(
        Cell{.x = 6, .y = 6}, Ruin{.kind = BuildingKind::House});

    run(1);

    EXPECT_EQ(world.get<FireCall>(fireman).target, first);
    (void)second;
}

TEST_F(RuinSystemTest, Update_SendsOneFiremanPerFireAndNoMore)
{
    const auto east = smoulder(
        Cell{.x = 6, .y = 2}, Ruin{.kind = BuildingKind::House});
    const auto west = smoulder(
        Cell{.x = 0, .y = 2}, Ruin{.kind = BuildingKind::House});

    const auto closer =
        walkerOf(Cell{.x = 1, .y = 2}, WalkerKind::Fireman);
    const auto further =
        walkerOf(Cell{.x = 2, .y = 2}, WalkerKind::Fireman);

    run(1);

    ASSERT_TRUE(world.has<FireCall>(closer));
    ASSERT_TRUE(world.has<FireCall>(further));
    EXPECT_EQ(world.get<FireCall>(closer).target, west);
    EXPECT_EQ(world.get<FireCall>(further).target, east);
}

TEST_F(RuinSystemTest, Update_LetsAFireBurnOutWithNobodyComing)
{
    const auto fire = smoulder(
        Cell{.x = 2, .y = 2}, Ruin{.kind = BuildingKind::House});

    run(static_cast<std::size_t>(kBurnDurationTicks) + 1);

    EXPECT_EQ(world.get<Ruin>(fire).state, RuinState::Debris);
    EXPECT_EQ(world.get<Ruin>(fire).ticksUntilOut, 0);
}

TEST_F(RuinSystemTest, Run_KeepsNoRuinOnADeadEntity)
{
    const auto entity = world.create();
    world.destroy(entity);
    world.add<Ruin>(entity, Ruin{.kind = BuildingKind::House});
    world.commit();

    EXPECT_FALSE(world.alive(entity));
    EXPECT_FALSE(world.has<Ruin>(entity));
}

TEST_F(RuinSystemTest, Run_KeepsNoFireCallOnADeadWalker)
{
    const auto fireman =
        walkerOf(Cell{.x = 1, .y = 1}, WalkerKind::Fireman);

    world.destroy(fireman);
    world.add<FireCall>(fireman, FireCall{});
    world.commit();

    EXPECT_FALSE(world.alive(fireman));
    EXPECT_FALSE(world.has<FireCall>(fireman));
}

TEST_F(RuinSystemTest, Update_FindsANearerFiremanLaterInCellOrder)
{
    const auto fire = smoulder(
        Cell{.x = 5, .y = 6}, Ruin{.kind = BuildingKind::House});

    walkerOf(Cell{.x = 1, .y = 9}, WalkerKind::Fireman);
    const auto nearer =
        walkerOf(Cell{.x = 5, .y = 5}, WalkerKind::Fireman);

    run(1);

    ASSERT_TRUE(world.has<FireCall>(nearer));
    EXPECT_EQ(world.get<FireCall>(nearer).target, fire);
}

TEST_F(RuinSystemTest, Update_LeavesANeighbourAloneBeforeTheSpreadDelay)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning,
             .ticksUntilOut = antwika::game::kBurnDurationTicks});
    standing(Cell{.x = 5, .y = 4}, BuildingKind::House);

    run(static_cast<std::size_t>(
        antwika::game::kSpreadDelayTicks - 1));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 1U);
}

TEST_F(RuinSystemTest, Update_SetsANeighbourAlightAfterTheSpreadDelay)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning,
             .ticksUntilOut = antwika::game::kBurnDurationTicks});
    const auto neighbour =
        standing(Cell{.x = 5, .y = 4}, BuildingKind::House);

    run(static_cast<std::size_t>(antwika::game::kSpreadDelayTicks));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 0U);
    EXPECT_FALSE(world.has<Building>(neighbour));

    std::size_t burning = 0;

    for (const auto entity : world.view<Ruin, Cell>())
    {
        if (world.get<Ruin>(entity).state == RuinState::Burning)
        {
            ++burning;
        }
    }

    EXPECT_EQ(burning, 2U);
}

TEST_F(RuinSystemTest, Update_LeavesABuildingOutOfReachStanding)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning,
             .ticksUntilOut = antwika::game::kBurnDurationTicks});
    standing(Cell{.x = 9, .y = 9}, BuildingKind::House);

    run(static_cast<std::size_t>(antwika::game::kSpreadDelayTicks));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 1U);
}

TEST_F(RuinSystemTest, Update_SpreadsNoFireOutOfDebris)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Debris, .ticksUntilOut = 0});
    standing(Cell{.x = 5, .y = 4}, BuildingKind::House);

    run(static_cast<std::size_t>(antwika::game::kSpreadDelayTicks));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 1U);
}

TEST_F(RuinSystemTest, Update_CatchesADiagonalNeighbourToo)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning,
             .ticksUntilOut = antwika::game::kBurnDurationTicks});
    standing(Cell{.x = 5, .y = 5}, BuildingKind::House);

    run(static_cast<std::size_t>(antwika::game::kSpreadDelayTicks));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 0U);
}

TEST_F(RuinSystemTest, Update_SpreadsNoFireWithTheDelayPutBeyondTheBurn)
{
    RuinSystem slow{
        built,
        kExtent,
        GameConfig{
            .burnDurationTicks = 40,
            .spreadDelayTicks = 100}};

    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning, .ticksUntilOut = 40});
    standing(Cell{.x = 5, .y = 4}, BuildingKind::House);

    for (std::size_t tick = 0; tick < 60; ++tick)
    {
        slow.update(world, tick);
        world.commit();
    }

    EXPECT_EQ((world.view<Building, Cell>().size()), 1U);
}

TEST_F(RuinSystemTest, Update_LeavesABuildingPastTheFireToTheEastAlone)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning,
             .ticksUntilOut = antwika::game::kBurnDurationTicks});
    standing(Cell{.x = 6, .y = 4}, BuildingKind::House);

    run(static_cast<std::size_t>(antwika::game::kSpreadDelayTicks));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 1U);
}

TEST_F(RuinSystemTest, Update_LeavesABuildingPastTheFireToTheWestAlone)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning,
             .ticksUntilOut = antwika::game::kBurnDurationTicks});
    standing(Cell{.x = 2, .y = 4}, BuildingKind::House);

    run(static_cast<std::size_t>(antwika::game::kSpreadDelayTicks));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 1U);
}

TEST_F(RuinSystemTest, Update_LeavesABuildingPastTheFireToTheSouthAlone)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning,
             .ticksUntilOut = antwika::game::kBurnDurationTicks});
    standing(Cell{.x = 4, .y = 6}, BuildingKind::House);

    run(static_cast<std::size_t>(antwika::game::kSpreadDelayTicks));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 1U);
}

TEST_F(RuinSystemTest, Update_LeavesABuildingPastTheFireToTheNorthAlone)
{
    smoulder(
        Cell{.x = 4, .y = 4},
        Ruin{.state = RuinState::Burning,
             .ticksUntilOut = antwika::game::kBurnDurationTicks});
    standing(Cell{.x = 4, .y = 2}, BuildingKind::House);

    run(static_cast<std::size_t>(antwika::game::kSpreadDelayTicks));

    world.commit();

    EXPECT_EQ((world.view<Building, Cell>().size()), 1U);
}
