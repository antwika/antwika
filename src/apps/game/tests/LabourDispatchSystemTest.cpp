#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/LabourDispatchSystem.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::Employment;
    using antwika::game::Household;
    using antwika::game::HousingLevel;
    using antwika::game::JobHolding;
    using antwika::game::kLabourPeriodTicks;
    using antwika::game::LabourDispatchSystem;
    using antwika::game::PathIndex;
    using antwika::game::setEmployment;
    using antwika::game::setHousehold;
    using antwika::game::Walker;
    using antwika::game::WalkerKind;
    using antwika::log::mocks::MockLogger;

    constexpr Cell kAt{.x = 4, .y = 4};

    class LabourDispatchSystemTest : public ::testing::Test
    {
    protected:
        Entity house(std::int32_t population)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, kAt);
            world.add<Building>(
                entity, Building{.kind = BuildingKind::House});
            world.commit();
            setHousehold(
                world,
                entity,
                Household{
                    .level = HousingLevel::Tent,
                    .population = population});
            world.commit();
            return entity;
        }

        [[nodiscard]] std::size_t labourersOut()
        {
            std::size_t count = 0;

            for (const auto entity : world.view<Walker, Cell>())
            {
                if (world.get<Walker>(entity).kind
                    == WalkerKind::Labourer)
                {
                    ++count;
                }
            }

            return count;
        }

        void run(std::size_t ticks = 1)
        {
            for (std::size_t tick = 0; tick < ticks; ++tick)
            {
                system.update(world, tick);
                world.commit();
            }
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        LabourDispatchSystem system{paths, antwika::game::GameConfig{}};
    };
}

TEST_F(LabourDispatchSystemTest, Run_SendsALabourerWhenHandsAreIdle)
{
    const auto home = house(3);
    paths.insert(Cell{.x = 4, .y = 3});

    run();

    ASSERT_EQ(labourersOut(), 1U);

    for (const auto entity : world.view<Walker, Cell>())
    {
        EXPECT_EQ(world.get<Walker>(entity).carried, 3);
        EXPECT_EQ(world.get<Walker>(entity).home, home);
    }
}

TEST_F(LabourDispatchSystemTest, Run_SendsNobodyWithNoRoadAtTheDoor)
{
    house(3);

    run();

    EXPECT_EQ(labourersOut(), 0U);
}

TEST_F(LabourDispatchSystemTest, Run_SendsNobodyFromAnEmptyHouse)
{
    house(0);
    paths.insert(Cell{.x = 4, .y = 3});

    run();

    EXPECT_EQ(labourersOut(), 0U);
}

TEST_F(LabourDispatchSystemTest, Run_SendsNobodyWithEverybodyWorking)
{
    const auto home = house(2);
    paths.insert(Cell{.x = 4, .y = 3});

    Employment employment;
    employment.jobs[0] =
        JobHolding{.workplace = home, .count = 2};
    setEmployment(world, home, employment);
    world.commit();

    run();

    EXPECT_EQ(labourersOut(), 0U);
}

TEST_F(LabourDispatchSystemTest, Run_SendsOneAtATimeOnItsCountdown)
{
    house(3);
    paths.insert(Cell{.x = 4, .y = 3});

    run();
    ASSERT_EQ(labourersOut(), 1U);

    run(static_cast<std::size_t>(kLabourPeriodTicks) / 2);

    EXPECT_EQ(labourersOut(), 1U);
}

TEST_F(LabourDispatchSystemTest, Run_SendsNobodyWhileOneIsOut)
{
    house(3);
    paths.insert(Cell{.x = 4, .y = 3});

    run();
    ASSERT_EQ(labourersOut(), 1U);

    run(static_cast<std::size_t>(kLabourPeriodTicks) + 2);

    EXPECT_EQ(labourersOut(), 1U);
}

TEST_F(LabourDispatchSystemTest, Run_SendsNobodyPastTheWalkerLimit)
{
    house(3);
    paths.insert(Cell{.x = 4, .y = 3});

    for (std::size_t walker = 0;
         walker < antwika::game::kWalkerLimit;
         ++walker)
    {
        const auto entity = world.create();
        world.add<Cell>(entity, Cell{.x = 0, .y = 0});
        world.add<Walker>(entity, Walker{});
    }

    world.commit();
    run();

    EXPECT_EQ(labourersOut(), 0U);
}

TEST_F(LabourDispatchSystemTest, Run_SendsNobodyWithNoFreeWalkerSlot)
{
    const auto home = house(3);
    paths.insert(Cell{.x = 4, .y = 3});

    auto building = world.get<Building>(home);

    for (std::size_t slot = 0;
         slot < antwika::game::kMaxWalkersOut;
         ++slot)
    {
        const auto occupant = world.create();
        world.add<Cell>(occupant, Cell{.x = 0, .y = 0});
        world.add<Walker>(occupant, Walker{});
        building.walkers[slot] = occupant;
    }

    world.set<Building>(home, building);
    world.commit();
    run();

    EXPECT_EQ(labourersOut(), 0U);
}
