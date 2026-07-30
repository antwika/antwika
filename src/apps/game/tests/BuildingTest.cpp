#include <gtest/gtest.h>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Resource.hpp"

using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::buildingFor;
using antwika::game::BuildTool;
using antwika::game::Cell;
using antwika::game::due;
using antwika::game::kDrainPeriodTicks;
using antwika::game::kRiskPeriodTicks;
using antwika::game::kSpawnPeriodTicks;
using antwika::game::kTicksPerSecond;
using antwika::game::newlyBuilt;
using antwika::game::orthogonallyAdjacent;
using antwika::game::Resource;
using antwika::game::Stock;
using antwika::game::stockedBy;

TEST(BuildToolTest, BuildingFor_NamesOneBuildingPerBuildingTool)
{
    EXPECT_EQ(buildingFor(BuildTool::House), BuildingKind::House);
    EXPECT_EQ(buildingFor(BuildTool::FoodSource), BuildingKind::FoodSource);
    EXPECT_EQ(buildingFor(BuildTool::WaterSource), BuildingKind::WaterSource);
    EXPECT_EQ(buildingFor(BuildTool::FireStation), BuildingKind::FireStation);
    EXPECT_EQ(
        buildingFor(BuildTool::ArchitectPost), BuildingKind::ArchitectPost);
}

// The road tool builds no building, which is what makes it the default.
TEST(BuildToolTest, BuildingFor_NamesNoBuildingForTheRoadTool)
{
    EXPECT_FALSE(buildingFor(BuildTool::Path).has_value());
}

TEST(StockTest, StockedBy_GivesAWaterSourceWaterAndEverythingElseFood)
{
    EXPECT_EQ(stockedBy(BuildingKind::WaterSource), Resource::Water);
    EXPECT_EQ(stockedBy(BuildingKind::FoodSource), Resource::Food);
    EXPECT_EQ(stockedBy(BuildingKind::House), Resource::Food);
    EXPECT_EQ(stockedBy(BuildingKind::FireStation), Resource::Food);
    EXPECT_EQ(stockedBy(BuildingKind::ArchitectPost), Resource::Food);
}

TEST(StockTest, Equality_ComparesEveryField)
{
    constexpr Stock one{.resource = Resource::Food, .held = 3, .capacity = 9};

    EXPECT_EQ(one, (Stock{.resource = Resource::Food, .held = 3,
                          .capacity = 9}));
    EXPECT_NE(one, (Stock{.resource = Resource::Water, .held = 3,
                          .capacity = 9}));
    EXPECT_NE(one, (Stock{.resource = Resource::Food, .held = 4,
                          .capacity = 9}));
    EXPECT_NE(one, (Stock{.resource = Resource::Food, .held = 3,
                          .capacity = 8}));
}

// The rule the requirement states outright.
TEST(BuildingTest, NewlyBuilt_StartsAtATenthOfCapacity)
{
    const auto house = newlyBuilt(BuildingKind::House);

    EXPECT_EQ(house.kind, BuildingKind::House);
    EXPECT_EQ(house.stock.capacity, 100);
    EXPECT_EQ(house.stock.held, 10);
    EXPECT_EQ(house.stock.resource, Resource::Food);
}

TEST(BuildingTest, NewlyBuilt_StartsWithNoRiskAndAFullSetOfCountdowns)
{
    const auto post = newlyBuilt(BuildingKind::ArchitectPost);

    EXPECT_EQ(post.fireRisk, 0);
    EXPECT_EQ(post.collapseRisk, 0);
    EXPECT_EQ(post.drainIn, kDrainPeriodTicks);
    EXPECT_EQ(post.riskIn, kRiskPeriodTicks);
    EXPECT_EQ(post.spawnIn, kSpawnPeriodTicks);
}

TEST(BuildingTest, NewlyBuilt_GivesAWaterSourceWater)
{
    EXPECT_EQ(
        newlyBuilt(BuildingKind::WaterSource).stock.resource,
        Resource::Water);
}

TEST(BuildingTest, Equality_ComparesEveryField)
{
    const auto one = newlyBuilt(BuildingKind::House);
    auto other = one;

    EXPECT_EQ(one, other);

    other.fireRisk = 1;
    EXPECT_NE(one, other);
}

// A building has to outlive the wait for its first walker.
TEST(BuildingTest, ANewBuildingOutlastsTheWaitForTheFirstSpawn)
{
    const auto house = newlyBuilt(BuildingKind::House);

    EXPECT_GT(house.stock.held * kDrainPeriodTicks, kSpawnPeriodTicks);
}

TEST(BuildingTest, TheSpawnPeriodIsAMinuteOfTheAssumedTickRate)
{
    EXPECT_EQ(kSpawnPeriodTicks, 60 * kTicksPerSecond);
}

TEST(CellAdjacencyTest, OrthogonallyAdjacent_IsTrueForTheFourNeighbours)
{
    constexpr Cell centre{.x = 4, .y = 7};

    EXPECT_TRUE(orthogonallyAdjacent(centre, Cell{.x = 4, .y = 6}));
    EXPECT_TRUE(orthogonallyAdjacent(centre, Cell{.x = 5, .y = 7}));
    EXPECT_TRUE(orthogonallyAdjacent(centre, Cell{.x = 4, .y = 8}));
    EXPECT_TRUE(orthogonallyAdjacent(centre, Cell{.x = 3, .y = 7}));
}

TEST(CellAdjacencyTest, OrthogonallyAdjacent_IsFalseForItselfAndDiagonals)
{
    constexpr Cell centre{.x = 4, .y = 7};

    EXPECT_FALSE(orthogonallyAdjacent(centre, centre));
    EXPECT_FALSE(orthogonallyAdjacent(centre, Cell{.x = 5, .y = 8}));
    EXPECT_FALSE(orthogonallyAdjacent(centre, Cell{.x = 3, .y = 6}));
    EXPECT_FALSE(orthogonallyAdjacent(centre, Cell{.x = 4, .y = 9}));
}

TEST(CountdownTest, Due_FiresOnTheLastTickAndReloads)
{
    std::int32_t remaining = 3;

    EXPECT_FALSE(due(remaining, 3));
    EXPECT_EQ(remaining, 2);

    EXPECT_FALSE(due(remaining, 3));
    EXPECT_EQ(remaining, 1);

    EXPECT_TRUE(due(remaining, 3));
    EXPECT_EQ(remaining, 3);
}

// One tick's period fires every tick, which is the degenerate case.
TEST(CountdownTest, Due_FiresEveryTickForAPeriodOfOne)
{
    std::int32_t remaining = 1;

    EXPECT_TRUE(due(remaining, 1));
    EXPECT_TRUE(due(remaining, 1));
}
