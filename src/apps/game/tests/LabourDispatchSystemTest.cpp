#include "antwika/game/LabourDispatchSystem.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

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
} // namespace

TEST_F(LabourDispatchSystemTest, SendsALabourerWithTheIdleHands)
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

TEST_F(LabourDispatchSystemTest, SendsNobodyWithNoRoadAtTheDoor)
{
    house(3);

    run();

    EXPECT_EQ(labourersOut(), 0U);
}

TEST_F(LabourDispatchSystemTest, SendsNobodyFromAnEmptyHouse)
{
    house(0);
    paths.insert(Cell{.x = 4, .y = 3});

    run();

    EXPECT_EQ(labourersOut(), 0U);
}

// A fully employed house has nothing to carry, so it sends nobody.
TEST_F(LabourDispatchSystemTest, SendsNobodyWithEverybodyWorking)
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

// One labourer out at a time, on the house's own countdown.
TEST_F(LabourDispatchSystemTest, SendsOneAtATimeOnItsOwnCountdown)
{
    house(3);
    paths.insert(Cell{.x = 4, .y = 3});

    run();
    ASSERT_EQ(labourersOut(), 1U);

    // The countdown was reset by the send, so nothing follows at once.
    run(static_cast<std::size_t>(kLabourPeriodTicks) / 2);

    EXPECT_EQ(labourersOut(), 1U);
}

// The send guard's other arms, one test each -- see the class comment.
TEST_F(LabourDispatchSystemTest, SendsNobodyWhileItsLabourerIsStillOut)
{
    house(3);
    paths.insert(Cell{.x = 4, .y = 3});

    run();
    ASSERT_EQ(labourersOut(), 1U);

    // The countdown has long expired and the walker is still out.
    run(static_cast<std::size_t>(kLabourPeriodTicks) + 2);

    EXPECT_EQ(labourersOut(), 1U);
}

TEST_F(LabourDispatchSystemTest, SendsNobodyPastTheWalkerLimit)
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

TEST_F(LabourDispatchSystemTest, SendsNobodyWithNoFreeWalkerSlot)
{
    const auto home = house(3);
    paths.insert(Cell{.x = 4, .y = 3});

    // Every slot filled by hand, with walkers that are not labourers.
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
