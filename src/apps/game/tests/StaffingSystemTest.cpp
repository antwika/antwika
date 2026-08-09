#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/StaffingSystem.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Workforce.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::Employment;
    using antwika::game::employedCount;
    using antwika::game::JobHolding;
    using antwika::game::kStaffDecayPeriodTicks;
    using antwika::game::setEmployment;
    using antwika::game::setStaff;
    using antwika::game::Staff;
    using antwika::game::staffCount;
    using antwika::game::StaffEntry;
    using antwika::game::StaffingSystem;
    using antwika::game::Walker;
    using antwika::game::WalkerKind;
    using antwika::log::mocks::MockLogger;

    class StaffingSystemTest : public ::testing::Test
    {
    protected:
        Entity stand(Cell at, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            return entity;
        }

        Entity labourer(Cell at, Entity home, std::int32_t carrying)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(
                entity,
                Walker{
                    .kind = WalkerKind::Labourer,
                    .carried = carrying,
                    .home = home});
            world.commit();
            return entity;
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
        StaffingSystem system{antwika::game::GameConfig{}};
    };
}

TEST_F(StaffingSystemTest, Run_GivesEveryWorkplaceAnEmptyLedger)
{
    const auto farm = stand(Cell{.x = 2, .y = 2}, BuildingKind::Farm);
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);

    run();

    EXPECT_TRUE(world.has<Staff>(farm));
    EXPECT_EQ(staffCount(world.get<Staff>(farm)), 0);

    EXPECT_FALSE(world.has<Staff>(house));
}

TEST_F(StaffingSystemTest, Run_MovesWorkforceIntoAWorkplace)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    labourer(Cell{.x = 2, .y = 3}, house, 3);

    run();

    const auto staff = world.get<Staff>(well);
    EXPECT_EQ(staffCount(staff), 1);
    EXPECT_EQ(staff.sources[0].house, house);

    const auto employment = world.get<Employment>(house);
    EXPECT_EQ(employedCount(employment), 1);
    EXPECT_EQ(employment.jobs[0].workplace, well);
}

TEST_F(StaffingSystemTest, Run_TakesOnlyWhatIsStillWanted)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto farm = stand(Cell{.x = 2, .y = 2}, BuildingKind::Farm);

    const auto walker = labourer(Cell{.x = 2, .y = 4}, house, 9);

    run();

    EXPECT_EQ(staffCount(world.get<Staff>(farm)), 4);
    EXPECT_EQ(world.get<Walker>(walker).carried, 5);
}

TEST_F(StaffingSystemTest, Run_TurnsAnEmptyLabourerForHome)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);
    (void)well;

    const auto walker = labourer(Cell{.x = 2, .y = 3}, house, 1);

    run();

    EXPECT_EQ(world.get<Walker>(walker).carried, 0);
    EXPECT_EQ(world.get<Walker>(walker).stepsUntilHome, 0);
}

TEST_F(StaffingSystemTest, Run_DecaysOnePersonBackHome)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    labourer(Cell{.x = 2, .y = 3}, house, 1);
    run();

    ASSERT_EQ(staffCount(world.get<Staff>(well)), 1);
    ASSERT_EQ(employedCount(world.get<Employment>(house)), 1);

    run(static_cast<std::size_t>(kStaffDecayPeriodTicks) + 1);

    EXPECT_EQ(staffCount(world.get<Staff>(well)), 0);
    EXPECT_EQ(employedCount(world.get<Employment>(house)), 0);
}

TEST_F(StaffingSystemTest, Run_DropsLedgerEntriesNamingTheDead)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    labourer(Cell{.x = 2, .y = 3}, house, 1);
    run();

    world.destroy(house);
    world.commit();
    run();

    EXPECT_EQ(staffCount(world.get<Staff>(well)), 0);
}

TEST_F(StaffingSystemTest, Run_DropsHoldingsNamingADeadWorkplace)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    labourer(Cell{.x = 2, .y = 3}, house, 1);
    run();

    world.destroy(well);
    world.commit();
    run();

    EXPECT_EQ(employedCount(world.get<Employment>(house)), 0);
}

TEST_F(StaffingSystemTest, Run_LeavesAFullWorkplaceAlone)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    Staff full;
    full.sources[0] =
        StaffEntry{.house = house, .count = 1};
    full.ticksUntilDecay = 10;
    setStaff(world, well, full);
    world.commit();

    const auto walker = labourer(Cell{.x = 2, .y = 3}, house, 2);

    run();

    EXPECT_EQ(staffCount(world.get<Staff>(well)), 1);
    EXPECT_EQ(world.get<Walker>(walker).carried, 2);
}

TEST_F(StaffingSystemTest, Run_DecayLeavesAnEmptyLedgerAlone)
{
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    run(static_cast<std::size_t>(kStaffDecayPeriodTicks) + 2);

    EXPECT_EQ(staffCount(world.get<Staff>(well)), 0);
}

TEST_F(StaffingSystemTest, Run_SkipsALabourerWithNoHome)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    const auto walker = labourer(Cell{.x = 2, .y = 3}, house, 2);

    world.destroy(house);
    world.commit();
    run();

    EXPECT_EQ(staffCount(world.get<Staff>(well)), 0);
    EXPECT_EQ(world.get<Walker>(walker).carried, 2);
}

TEST_F(StaffingSystemTest, Run_EmptiesIntoTwoWorkplacesOnARoad)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto northWell =
        stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);
    const auto southWell =
        stand(Cell{.x = 2, .y = 4}, BuildingKind::Well);

    const auto walker = labourer(Cell{.x = 2, .y = 3}, house, 2);

    run();

    EXPECT_EQ(staffCount(world.get<Staff>(northWell)), 1);
    EXPECT_EQ(staffCount(world.get<Staff>(southWell)), 1);
    EXPECT_EQ(world.get<Walker>(walker).carried, 0);
}

TEST_F(StaffingSystemTest, Run_LeavesAFullJobLedgerAlone)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);
    (void)well;

    Employment full;

    for (std::size_t slot = 0; slot < antwika::game::kMaxJobs; ++slot)
    {
        full.jobs[slot] = JobHolding{
            .workplace = house, .count = 1};
    }

    setEmployment(world, house, full);
    world.commit();

    const auto walker = labourer(Cell{.x = 2, .y = 3}, house, 2);

    run();

    EXPECT_EQ(world.get<Walker>(walker).carried, 2);
}

TEST_F(StaffingSystemTest, Run_KeepsTwoHousesApartOnALedger)
{
    const auto first = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto second =
        stand(Cell{.x = 8, .y = 8}, BuildingKind::House);
    const auto farm = stand(Cell{.x = 2, .y = 2}, BuildingKind::Farm);

    labourer(Cell{.x = 2, .y = 4}, first, 2);
    labourer(Cell{.x = 3, .y = 3}, second, 2);

    run();

    const auto staff = world.get<Staff>(farm);
    EXPECT_EQ(staffCount(staff), 4);
    EXPECT_EQ(staff.sources[0].house, first);
    EXPECT_EQ(staff.sources[1].house, second);
    EXPECT_EQ(employedCount(world.get<Employment>(first)), 2);
    EXPECT_EQ(employedCount(world.get<Employment>(second)), 2);
}

TEST_F(StaffingSystemTest, Run_GrowsAHousesOwnSlot)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto farm = stand(Cell{.x = 2, .y = 2}, BuildingKind::Farm);

    labourer(Cell{.x = 2, .y = 4}, house, 1);
    run();
    labourer(Cell{.x = 2, .y = 4}, house, 1);
    run();

    const auto staff = world.get<Staff>(farm);
    EXPECT_EQ(staffCount(staff), 2);
    EXPECT_EQ(staff.sources[0].count, 2);
    EXPECT_EQ(staff.sources[1].house, kNullEntity);
}

TEST_F(StaffingSystemTest, Run_ReleasesTheRightJobHolding)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);
    const auto farm = stand(Cell{.x = 4, .y = 8}, BuildingKind::Farm);

    Employment jobs;
    jobs.jobs[0] = JobHolding{.workplace = farm, .count = 1};
    jobs.jobs[1] = JobHolding{.workplace = well, .count = 1};
    setEmployment(world, house, jobs);

    Staff staff;
    staff.sources[0] = StaffEntry{.house = house, .count = 1};
    staff.ticksUntilDecay = 0;
    setStaff(world, well, staff);
    world.commit();

    run();

    const auto employment = world.get<Employment>(house);
    EXPECT_EQ(employedCount(employment), 1);
    EXPECT_EQ(employment.jobs[0].workplace, farm);
}

TEST_F(StaffingSystemTest, Run_DecaysWithNoMatchingHolding)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    Staff staff;
    staff.sources[0] = StaffEntry{.house = house, .count = 1};
    staff.ticksUntilDecay = 0;
    setStaff(world, well, staff);
    world.commit();

    run();

    EXPECT_EQ(staffCount(world.get<Staff>(well)), 0);
    EXPECT_EQ(employedCount(world.get<Employment>(house)), 0);
}

TEST_F(StaffingSystemTest, Run_DecayLeavesASecondWorkerFromOneHouse)
{
    const auto house = stand(Cell{.x = 6, .y = 6}, BuildingKind::House);
    const auto well = stand(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    Employment jobs;
    jobs.jobs[0] = JobHolding{.workplace = well, .count = 2};
    setEmployment(world, house, jobs);

    Staff staff;
    staff.sources[0] = StaffEntry{.house = house, .count = 2};
    staff.ticksUntilDecay = 0;
    setStaff(world, well, staff);
    world.commit();

    run();

    const auto shed = world.get<Staff>(well);
    const auto employment = world.get<Employment>(house);

    EXPECT_EQ(shed.sources[0].count, 1);
    EXPECT_EQ(shed.sources[0].house, house);
    EXPECT_EQ(employment.jobs[0].count, 1);
    EXPECT_EQ(employment.jobs[0].workplace, well);
}
