#include "antwika/game/PopulationSystem.hpp"

#include <cstddef>
#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/PathIndex.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::DesirabilityField;
    using antwika::game::Household;
    using antwika::game::householdOf;
    using antwika::game::HousingLevel;
    using antwika::game::kSettlerPeriodTicks;
    using antwika::game::PathIndex;
    using antwika::game::populationAt;
    using antwika::game::populationCapacityOf;
    using antwika::game::PopulationSystem;
    using antwika::game::requirementOf;
    using antwika::game::setHousehold;
    using antwika::log::mocks::MockLogger;

    constexpr Cell kAt{.x = 4, .y = 4};

    // One house, one road, one field, built fresh per case.
    class Scene
    {
    public:
        explicit Scene(BuildingKind kind = BuildingKind::House)
        {
            house = world.create();
            world.add<Cell>(house, kAt);
            world.add<Building>(house, Building{.kind = kind});
            world.commit();
        }

        void pave()
        {
            paths.insert(Cell{.x = kAt.x, .y = kAt.y - 1});
        }

        void settle(Household household)
        {
            setHousehold(world, house, household);
            world.commit();
        }

        void run(std::int32_t ticks)
        {
            for (std::int32_t tick = 0; tick < ticks; ++tick)
            {
                system.update(world, static_cast<std::size_t>(tick));
                world.commit();
            }
        }

        [[nodiscard]] std::int32_t people()
        {
            return populationAt(world, house);
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        DesirabilityField field;
        PopulationSystem system{paths, field};
        Entity house{};
    };
} // namespace

// Everybody arrives on foot, so a block nothing reaches houses nobody.
TEST(PopulationSystemTest, Update_HousesNobodyWhereNoRoadRunsBeside)
{
    Scene scene;

    scene.run(4 * kSettlerPeriodTicks);

    EXPECT_EQ(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_MovesOnePersonInEachPeriod)
{
    Scene scene;
    scene.pave();

    scene.run(kSettlerPeriodTicks);
    EXPECT_EQ(scene.people(), 1);

    scene.run(kSettlerPeriodTicks);
    EXPECT_EQ(scene.people(), 2);
}

// Not a tick sooner, which is what the countdown is for.
TEST(PopulationSystemTest, Update_MovesNobodyInBeforeThePeriodIsUp)
{
    Scene scene;
    scene.pave();

    scene.run(kSettlerPeriodTicks - 1);

    EXPECT_EQ(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_FillsATentAndStopsAtItsCapacity)
{
    Scene scene;
    scene.pave();

    const auto capacity = populationCapacityOf(HousingLevel::Tent);
    scene.run(kSettlerPeriodTicks * (capacity + 4));

    EXPECT_EQ(scene.people(), capacity);
}

// A bigger house holds more people.
// Which is the loop this whole application is arranged around.
TEST(PopulationSystemTest, Update_FillsAGrownHouseToItsOwnCapacity)
{
    Scene scene;
    scene.pave();
    scene.field[kAt] =
        requirementOf(HousingLevel::Cottage).desirability;
    scene.settle(Household{.level = HousingLevel::Cottage});

    const auto capacity = populationCapacityOf(HousingLevel::Cottage);
    scene.run(kSettlerPeriodTicks * (capacity + 2));

    EXPECT_EQ(scene.people(), capacity);
}

// A clay pit next door is what makes a district's desirability negative.
TEST(PopulationSystemTest, Update_EmptiesAHouseOnGroundBelowItsThreshold)
{
    Scene scene;
    scene.pave();
    scene.field[kAt] =
        requirementOf(HousingLevel::Hovel).desirability - 1;
    scene.settle(
        Household{.level = HousingLevel::Hovel, .population = 3});

    scene.run(kSettlerPeriodTicks * 2);

    EXPECT_EQ(scene.people(), 1);
}

TEST(PopulationSystemTest, Update_EmptiesAHouseDownToNobodyAndNoFurther)
{
    Scene scene;
    scene.pave();
    scene.field[kAt] =
        requirementOf(HousingLevel::Hovel).desirability - 1;
    scene.settle(
        Household{.level = HousingLevel::Hovel, .population = 1});

    scene.run(kSettlerPeriodTicks * 5);

    EXPECT_EQ(scene.people(), 0);
}

// Which is what a house that has just devolved is.
TEST(PopulationSystemTest, Update_ThinsAHouseHoldingMoreThanItsCapacity)
{
    Scene scene;
    scene.pave();

    const auto over = populationCapacityOf(HousingLevel::Tent) + 3;
    scene.settle(Household{.population = over});

    scene.run(kSettlerPeriodTicks);

    EXPECT_EQ(scene.people(), over - 1);
}

// A well is not somewhere anybody lives.
TEST(PopulationSystemTest, Update_HousesNobodyInAKindNobodyLivesIn)
{
    Scene scene(BuildingKind::Well);
    scene.pave();

    scene.run(kSettlerPeriodTicks * 2);

    EXPECT_FALSE(scene.world.has<Household>(scene.house));
}

// Growing into a bigger house is not a reason to turn round at the door.
TEST(PopulationSystemTest, Update_LeavesTheOtherTwoCountdownsAlone)
{
    Scene scene;
    scene.pave();

    scene.run(kSettlerPeriodTicks);

    const auto household = householdOf(scene.world, scene.house);
    EXPECT_EQ(
        household.ticksUntilEvolve, Household{}.ticksUntilEvolve);
    EXPECT_EQ(
        household.ticksUntilDevolve, Household{}.ticksUntilDevolve);
    EXPECT_EQ(household.ticksUntilSettler, kSettlerPeriodTicks);
}
