#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/RatingsSystem.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/CityRatings.hpp"
#include "antwika/game/Household.hpp"

namespace
{
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::CityRatings;
    using antwika::game::Household;
    using antwika::game::RatingsSystem;
    using antwika::game::setHousehold;
    using antwika::log::mocks::MockLogger;
}

TEST(RatingsSystemTest, Update_WritesWhatTheCityRatesIntoTheOneItKeeps)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};
    CityRatings ratings;
    RatingsSystem system{ratings};

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 1, .y = 1});
    world.add<Building>(home, Building{.kind = BuildingKind::House});
    world.commit();
    setHousehold(world, home, Household{.population = 6});
    world.commit();

    system.update(world, 0);

    EXPECT_EQ(ratings.population, 6);
}

TEST(RatingsSystemTest, Update_ForgetsACityThatHasBeenTornDown)
{
    ::testing::NiceMock<MockLogger> logger;
    World world{logger};
    CityRatings ratings;
    RatingsSystem system{ratings};

    const auto home = world.create();
    world.add<Cell>(home, Cell{.x = 1, .y = 1});
    world.add<Building>(home, Building{.kind = BuildingKind::House});
    world.commit();
    setHousehold(world, home, Household{.population = 6});
    world.commit();

    system.update(world, 0);
    ASSERT_EQ(ratings.population, 6);

    world.destroy(home);
    world.commit();

    system.update(world, 1);

    EXPECT_EQ(ratings, CityRatings{});
}
