#include "antwika/game/CityRatings.hpp"

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
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Workforce.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::CityRatings;
    using antwika::game::Coverage;
    using antwika::game::Household;
    using antwika::game::HousingLevel;
    using antwika::game::kCoverageFull;
    using antwika::game::ratingsOf;
    using antwika::game::setCoverage;
    using antwika::game::setHousehold;
    using antwika::game::setWorkforce;
    using antwika::game::Workforce;
    using antwika::game::workersWantedBy;
    using antwika::log::mocks::MockLogger;

    class Scene
    {
    public:
        Entity put(Cell at, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            return entity;
        }

        Entity house(Cell at, Household household)
        {
            const auto entity = put(at, BuildingKind::House);
            setHousehold(world, entity, household);
            world.commit();
            return entity;
        }

        void employ(Entity entity, std::int32_t people)
        {
            setWorkforce(world, entity, Workforce{.employed = people});
            world.commit();
        }

        void serve(Entity entity, std::size_t services)
        {
            Coverage coverage;

            for (std::size_t slot = 0; slot < services; ++slot)
            {
                coverage.ticksLeft[slot] = kCoverageFull;
            }

            setCoverage(world, entity, coverage);
            world.commit();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
    };
} // namespace

// A default CityRatings is exactly what an empty city rates.
// Which is what makes the default worth having at all.
TEST(CityRatingsTest, RatingsOf_RatesAnEmptyCityAtNothingAtAll)
{
    Scene scene;

    EXPECT_EQ(ratingsOf(scene.world), CityRatings{});
}

TEST(CityRatingsTest, RatingsOf_CountsEverybodyLivingInTheCity)
{
    Scene scene;
    scene.house(Cell{.x = 0, .y = 0}, Household{.population = 4});
    scene.house(Cell{.x = 1, .y = 0}, Household{.population = 3});

    EXPECT_EQ(ratingsOf(scene.world).population, 7);
}

// A house is not a workplace, so a city of houses has no jobs at all.
TEST(CityRatingsTest, RatingsOf_RatesACityWithNoJobsAtNoEmployment)
{
    Scene scene;
    scene.house(Cell{.x = 0, .y = 0}, Household{.population = 4});

    EXPECT_EQ(ratingsOf(scene.world).employment, 0);
}

TEST(CityRatingsTest, RatingsOf_ReportsTheShareOfTheCitysJobsThatAreFilled)
{
    Scene scene;
    const auto farm = scene.put(Cell{.x = 2, .y = 0}, BuildingKind::Farm);
    const auto well = scene.put(Cell{.x = 4, .y = 0}, BuildingKind::Well);

    scene.employ(farm, 1);
    scene.employ(well, 0);

    // One of the farm's four, and none of the well's one.
    const auto jobs = workersWantedBy(BuildingKind::Farm)
        + workersWantedBy(BuildingKind::Well);
    EXPECT_EQ(ratingsOf(scene.world).employment, 100 / jobs);
}

// Which is exactly what such a workplace does -- see LabourQuery.hpp.
TEST(CityRatingsTest, RatingsOf_CountsAWorkplaceNothingHasAllocatedToYet)
{
    Scene scene;
    scene.put(Cell{.x = 2, .y = 0}, BuildingKind::Farm);

    EXPECT_EQ(ratingsOf(scene.world).employment, 100);
}

TEST(CityRatingsTest, RatingsOf_RatesACityOfTentsAtNoHousingAtAll)
{
    Scene scene;
    scene.house(Cell{.x = 0, .y = 0}, Household{});

    EXPECT_EQ(ratingsOf(scene.world).averageHousingLevel, 0);
}

// Hundredths of a tier, so two houses one tier apart average between.
TEST(CityRatingsTest, RatingsOf_AveragesTheHousingTierInHundredths)
{
    Scene scene;
    scene.house(
        Cell{.x = 0, .y = 0}, Household{.level = HousingLevel::Tent});
    scene.house(
        Cell{.x = 1, .y = 0}, Household{.level = HousingLevel::Hovel});

    EXPECT_EQ(ratingsOf(scene.world).averageHousingLevel, 100);
}

TEST(CityRatingsTest, RatingsOf_ReportsTheShareOfServicesReachingHouses)
{
    Scene scene;
    const auto home = scene.house(Cell{.x = 0, .y = 0}, Household{});

    scene.serve(home, 1);

    EXPECT_EQ(
        ratingsOf(scene.world).serviceReach,
        100 / static_cast<std::int32_t>(antwika::game::kServiceCount));
}

TEST(CityRatingsTest, RatingsOf_ReportsFullReachForAFullyServedHouse)
{
    Scene scene;
    const auto home = scene.house(Cell{.x = 0, .y = 0}, Household{});

    scene.serve(home, antwika::game::kServiceCount);

    EXPECT_EQ(ratingsOf(scene.world).serviceReach, 100);
}

// A well being watered says nothing about how the city houses people.
TEST(CityRatingsTest, RatingsOf_CountsServiceReachOverHousesAlone)
{
    Scene scene;
    const auto well = scene.put(Cell{.x = 2, .y = 0}, BuildingKind::Well);

    scene.serve(well, antwika::game::kServiceCount);

    EXPECT_EQ(ratingsOf(scene.world).serviceReach, 0);
}

// Every sum is commutative, so the answer is a function of the set.
TEST(CityRatingsTest, RatingsOf_IsIdenticalUnderTwoCreationOrders)
{
    Scene forwards;
    forwards.house(Cell{.x = 0, .y = 0}, Household{.population = 4});
    forwards.house(
        Cell{.x = 1, .y = 0},
        Household{.level = HousingLevel::Shack, .population = 2});
    forwards.employ(
        forwards.put(Cell{.x = 3, .y = 0}, BuildingKind::Farm), 2);

    Scene backwards;
    backwards.employ(
        backwards.put(Cell{.x = 3, .y = 0}, BuildingKind::Farm), 2);
    backwards.house(
        Cell{.x = 1, .y = 0},
        Household{.level = HousingLevel::Shack, .population = 2});
    backwards.house(Cell{.x = 0, .y = 0}, Household{.population = 4});

    EXPECT_EQ(ratingsOf(forwards.world), ratingsOf(backwards.world));
}
