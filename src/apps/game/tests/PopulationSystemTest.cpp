#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <algorithm>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Journey.hpp"
#include "antwika/game/PopulationSystem.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WalkerSystem.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Service.hpp"

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
    using antwika::game::kMigrantPeriodTicks;
    using antwika::game::kTicksPerSecond;
    using antwika::game::GridExtent;
using antwika::game::GameConfig;
using antwika::game::PathIndex;
    using antwika::game::populationAt;
    using antwika::game::populationCapacityOf;
    using antwika::game::PopulationSystem;
    using antwika::game::requirementOf;
    using antwika::game::setHousehold;
    using antwika::game::Walker;
using antwika::log::mocks::MockLogger;

    constexpr Cell kAt{.x = 4, .y = 4};
    constexpr GridExtent kExtent{.width = 9, .height = 9};

    class Scene final
    {
    public:
        explicit Scene(BuildingKind kind = BuildingKind::House)
        {
            house = world.create();
            world.add<Cell>(house, kAt);
            world.add<Building>(house, Building{.kind = kind});
            world.commit();

            water();
        }

        void water(std::int32_t left = antwika::game::kCoverageFull)
        {
            antwika::game::Coverage coverage;
            coverage.ticksLeft[antwika::game::serviceIndex(
                antwika::game::Service::Water)] = left;
            antwika::game::setCoverage(world, house, coverage);
            world.commit();
        }

        void pave()
        {
            for (std::int32_t x = 0; x <= kAt.x; ++x)
            {
                paths.insert(Cell{.x = x, .y = kAt.y - 1});
            }
        }

        void paveDoor()
        {
            paths.insert(Cell{.x = kAt.x, .y = kAt.y - 1});
        }

        void wallIn()
        {
            paths.insert(Cell{.x = kAt.x, .y = kAt.y - 1});

            (void)built.insert(
                kAt, antwika::game::footprintOf(BuildingKind::House));

            for (const auto around : {
                     Cell{.x = kAt.x - 1, .y = kAt.y - 1},
                     Cell{.x = kAt.x + 1, .y = kAt.y - 1},
                     Cell{.x = kAt.x, .y = kAt.y - 2}})
            {
                (void)built.insert(
                    around,
                    antwika::game::footprintOf(BuildingKind::Well));
            }
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
                walkers.update(world, static_cast<std::size_t>(clock));
                world.commit();
                system.update(world, static_cast<std::size_t>(clock));
                world.commit();
                ++clock;
            }
        }

        [[nodiscard]] Entity walkTo(Entity house)
        {
            const auto coming = world.create();
            world.add<Cell>(coming, Cell{.x = 0, .y = kAt.y - 1});
            world.add<Walker>(
                coming,
                Walker{
                    .kind = antwika::game::WalkerKind::Migrant,
                    .carried = 1});
            world.add<antwika::game::Journey>(
                coming,
                antwika::game::Journey{.towards = kAt, .house = house});
            world.commit();

            return coming;
        }

        [[nodiscard]] std::int32_t walking()
        {
            std::int32_t count = 0;

            for (const auto entity : world.view<Walker>())
            {
                if (world.get<Walker>(entity).kind
                    == antwika::game::WalkerKind::Migrant)
                {
                    ++count;
                }
            }

            return count;
        }

        [[nodiscard]] std::int32_t people()
        {
            return populationAt(world, house);
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        DesirabilityField field;
        antwika::game::BuildingIndex built;
        PopulationSystem system{paths, built, field, kExtent, GameConfig{}};
        antwika::game::WalkerSystem walkers{paths, built, kExtent};
        Entity house{};
        std::int32_t clock = 0;
    };
}

TEST(PopulationSystemTest, Update_HousesNobodyWhereNoRoadRunsBeside)
{
    Scene scene;

    scene.run(4 * kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_HousesNobodyWithNoWayIntoTheCity)
{
    Scene scene;
    scene.wallIn();

    scene.run(4 * kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), 0);
    EXPECT_EQ(scene.walking(), 0);
}

TEST(PopulationSystemTest, Update_SendsOneWaveAndNoMoreUntilItLands)
{
    Scene scene;
    scene.pave();

    scene.run(1);

    EXPECT_EQ(scene.walking(), 1);

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.walking(), 1);
}

TEST(PopulationSystemTest, Update_SendsAWaveCarryingTheRoomTheHouseHas)
{
    Scene scene;
    scene.pave();

    scene.run(1);

    ASSERT_EQ(scene.walking(), 1);

    for (const auto entity : scene.world.view<Walker>())
    {
        EXPECT_EQ(
            scene.world.get<Walker>(entity).carried,
            populationCapacityOf(HousingLevel::Tent));
    }
}

TEST(PopulationSystemTest, Update_FillsAnEmptyTentFromTheOneWaveItSent)
{
    Scene scene;
    scene.pave();

    scene.run(kTicksPerSecond * 2);

    EXPECT_EQ(scene.people(), populationCapacityOf(HousingLevel::Tent));
}

TEST(PopulationSystemTest, Update_SendsForNobodyBetweenTwoMigrantPeriods)
{
    Scene scene;
    scene.pave();

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.walking(), 1);
}

TEST(PopulationSystemTest, Update_HousesNobodyUntilAMigrantHasWalkedIn)
{
    Scene scene;
    scene.pave();

    scene.run(1);

    EXPECT_EQ(scene.walking(), 1);
    EXPECT_EQ(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_HousesSomebodyOverBareGround)
{
    Scene scene;

    scene.paveDoor();

    scene.run(kMigrantPeriodTicks * 3);

    EXPECT_GT(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_WalksAnOverflowOutOverBareGround)
{
    Scene scene;
    scene.paveDoor();

    const auto over = populationCapacityOf(HousingLevel::Tent) + 2;
    scene.settle(Household{.population = over});

    scene.run(kMigrantPeriodTicks * 12);

    EXPECT_EQ(scene.people(), populationCapacityOf(HousingLevel::Tent));
    EXPECT_EQ(scene.walking(), 0);
}

TEST(PopulationSystemTest, Update_LetsAMigrantInAtTheEdgeOfTheMap)
{
    Scene scene;
    scene.pave();

    scene.run(1);

    ASSERT_EQ(scene.walking(), 1);

    for (const auto entity : scene.world.view<Walker, Cell>())
    {
        const auto at = scene.world.get<Cell>(entity);

        EXPECT_LE(
            std::min(
                {at.x, at.y, kExtent.width - 1 - at.x,
                 kExtent.height - 1 - at.y}),
            1);
    }
}

TEST(PopulationSystemTest, Update_FillsATentAndStopsAtItsCapacity)
{
    Scene scene;
    scene.pave();

    const auto capacity = populationCapacityOf(HousingLevel::Tent);
    scene.run(kMigrantPeriodTicks * (capacity + 8));

    EXPECT_EQ(scene.people(), capacity);
}

TEST(PopulationSystemTest, Update_FillsAGrownHouseToItsOwnCapacity)
{
    Scene scene;
    scene.pave();
    scene.field[kAt] =
        requirementOf(HousingLevel::Cottage).desirability;
    scene.settle(Household{.level = HousingLevel::Cottage});

    const auto capacity = populationCapacityOf(HousingLevel::Cottage);
    scene.run(kMigrantPeriodTicks * (capacity + 8));

    EXPECT_EQ(scene.people(), capacity);
}

TEST(PopulationSystemTest, Update_EmptiesAHouseOnGroundBelowItsThreshold)
{
    Scene scene;
    scene.pave();
    scene.field[kAt] =
        requirementOf(HousingLevel::Hovel).desirability - 1;
    scene.settle(
        Household{.level = HousingLevel::Hovel, .population = 3});

    scene.run(kMigrantPeriodTicks * 2);

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

    scene.run(kMigrantPeriodTicks * 5);

    EXPECT_EQ(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_ThinsAHouseHoldingMoreThanItsCapacity)
{
    Scene scene;
    scene.pave();

    const auto over = populationCapacityOf(HousingLevel::Tent) + 3;
    scene.settle(Household{.population = over});

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), over - 1);

    EXPECT_EQ(scene.walking(), 1);
}

TEST(PopulationSystemTest, Update_WalksACrowdedHousesOverflowOffTheMap)
{
    Scene scene;
    scene.pave();

    const auto capacity = populationCapacityOf(HousingLevel::Tent);
    scene.settle(Household{.population = capacity + 2});

    scene.run(kMigrantPeriodTicks * 12);

    EXPECT_EQ(scene.people(), capacity);
    EXPECT_EQ(scene.walking(), 0);
}

TEST(PopulationSystemTest, Update_HousesNobodyInAKindNobodyLivesIn)
{
    Scene scene(BuildingKind::Well);
    scene.pave();

    scene.run(kMigrantPeriodTicks * 2);

    EXPECT_FALSE(scene.world.has<Household>(scene.house));
}

TEST(PopulationSystemTest, Update_LeavesTheOtherTwoCountdownsAlone)
{
    Scene scene;
    scene.pave();

    scene.run(kMigrantPeriodTicks);

    const auto household = householdOf(scene.world, scene.house);
    EXPECT_EQ(
        household.ticksUntilEvolve, Household{}.ticksUntilEvolve);
    EXPECT_EQ(
        household.ticksUntilDevolve, Household{}.ticksUntilDevolve);
}

TEST(PopulationSystemTest, Update_SendsForNobodyAtTheWalkerCap)
{
    Scene scene;
    scene.pave();

    for (std::size_t index = 0; index < antwika::game::kWalkerLimit;
         ++index)
    {
        const auto filler = scene.world.create();
        scene.world.add<Cell>(filler, Cell{.x = 0, .y = kAt.y - 1});
        scene.world.add<Walker>(filler, Walker{});
    }

    scene.world.commit();
    scene.run(kMigrantPeriodTicks * 2);

    EXPECT_EQ(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_WalksNobodyOutAtTheWalkerCap)
{
    Scene scene;
    scene.pave();

    const auto over = populationCapacityOf(HousingLevel::Tent) + 3;
    scene.settle(Household{.population = over});

    for (std::size_t index = 0; index < antwika::game::kWalkerLimit;
         ++index)
    {
        const auto filler = scene.world.create();
        scene.world.add<Cell>(filler, Cell{.x = 0, .y = kAt.y - 1});
        scene.world.add<Walker>(filler, Walker{});
    }

    scene.world.commit();
    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), over - 1);
    EXPECT_EQ(scene.walking(), 0);
}

TEST(PopulationSystemTest, Update_ThinsACrowdedHouseWithNoWayOut)
{
    Scene scene;
    scene.wallIn();

    const auto over = populationCapacityOf(HousingLevel::Tent) + 3;
    scene.settle(Household{.population = over});

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), over - 1);
    EXPECT_EQ(scene.walking(), 0);
}

TEST(PopulationSystemTest, Update_ThinsACrowdedHouseWithNoRoadBeside)
{
    Scene scene;

    const auto over = populationCapacityOf(HousingLevel::Tent) + 3;
    scene.settle(Household{.population = over});

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), over - 1);
    EXPECT_EQ(scene.walking(), 0);
}

TEST(PopulationSystemTest, Update_WalksAnOverflowToAHouseWithRoom)
{
    Scene scene;
    scene.pave();

    const auto over = populationCapacityOf(HousingLevel::Tent) + 1;
    scene.settle(Household{.population = over});

    const auto spare = scene.world.create();
    scene.world.add<Cell>(spare, Cell{.x = 1, .y = kAt.y});
    scene.world.add<Building>(
        spare, Building{.kind = BuildingKind::House});
    scene.world.commit();

    scene.run(kMigrantPeriodTicks);

    bool bound = false;

    for (const auto entity :
         scene.world.view<Walker, antwika::game::Journey>())
    {
        bound = bound
            || scene.world.get<antwika::game::Journey>(entity).house
                == spare;
    }

    EXPECT_TRUE(bound);
}

TEST(PopulationSystemTest, Update_AdmitsNobodyForAJourneyBoundNowhere)
{
    Scene scene;
    scene.pave();

    const auto stray = scene.world.create();
    scene.world.add<Cell>(stray, Cell{.x = kAt.x, .y = kAt.y - 1});
    scene.world.add<Walker>(
        stray,
        Walker{.kind = antwika::game::WalkerKind::Migrant});
    scene.world.add<antwika::game::Journey>(
        stray, antwika::game::Journey{.towards = Cell{.x = 0, .y = 0}});
    scene.world.commit();

    scene.run(1);

    EXPECT_EQ(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_SendsForNobodyWhileAWaveIsWalking)
{
    Scene scene;
    scene.pave();

    static_cast<void>(scene.walkTo(scene.house));

    scene.run(1);

    EXPECT_EQ(scene.walking(), 1);
}

TEST(PopulationSystemTest, Update_AdmitsNobodyForAJourneyWhoseHouseWent)
{
    Scene scene;
    scene.pave();

    const auto stray = scene.world.create();
    scene.world.add<Cell>(stray, Cell{.x = kAt.x, .y = kAt.y - 1});
    scene.world.add<Walker>(
        stray,
        Walker{.kind = antwika::game::WalkerKind::Migrant});
    scene.world.add<antwika::game::Journey>(
        stray,
        antwika::game::Journey{
            .towards = kAt,
            .house = static_cast<antwika::ecs::Entity>(99)});
    scene.world.commit();

    scene.system.update(scene.world, 0);
    scene.world.commit();

    EXPECT_EQ(scene.people(), 0);
    EXPECT_TRUE(scene.world.alive(stray));
}

TEST(PopulationSystemTest, Update_AsksAgainForNobodyOnceTheTentIsFull)
{
    Scene scene;
    scene.pave();

    scene.run(kMigrantPeriodTicks * 4);

    EXPECT_EQ(scene.walking(), 0);
}

TEST(PopulationSystemTest, Update_WalksPeopleOutOfAHouseGoneDry)
{
    Scene scene;
    scene.pave();
    scene.settle(Household{.population = 2});
    scene.water(0);

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), 1);
    EXPECT_EQ(scene.walking(), 1);

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), 0);
}

TEST(PopulationSystemTest, Update_WalksPeopleOutOfAStarvedHouse)
{
    Scene scene;
    scene.pave();
    scene.settle(Household{.population = 2});

    auto building = scene.world.get<Building>(scene.house);
    building.stock = {};
    scene.world.set<Building>(scene.house, building);
    scene.world.commit();

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), 1);
    EXPECT_EQ(scene.walking(), 1);
}

TEST(PopulationSystemTest, Update_SendsForNobodyWhileTheHouseIsDry)
{
    Scene scene;
    scene.pave();
    scene.water(0);

    scene.run(4 * kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), 0);
    EXPECT_EQ(scene.walking(), 0);
}

TEST(PopulationSystemTest, Update_SendsADryHousesPeopleOffTheMap)
{
    Scene scene;
    scene.pave();
    scene.settle(Household{.population = 1});
    scene.water(0);

    const auto vacancy = scene.world.create();
    scene.world.add<Cell>(vacancy, Cell{.x = 6, .y = 6});
    scene.world.add<Building>(
        vacancy, Building{.kind = BuildingKind::House});
    scene.world.commit();

    scene.run(kMigrantPeriodTicks);

    EXPECT_EQ(scene.people(), 0);
    ASSERT_EQ(scene.walking(), 1);

    for (const auto walker :
         scene.world.view<Walker, antwika::game::Journey>())
    {
        EXPECT_EQ(
            scene.world.get<antwika::game::Journey>(walker).house,
            antwika::ecs::kNullEntity);
    }
}

TEST(PopulationSystemTest, Update_StillMovesACrowdedHousesOverflowIn)
{
    Scene scene;
    scene.pave();
    scene.settle(
        Household{
            .population =
                populationCapacityOf(HousingLevel::Tent) + 1});

    const auto vacancy = scene.world.create();
    scene.world.add<Cell>(vacancy, Cell{.x = 6, .y = 6});
    scene.world.add<Building>(
        vacancy, Building{.kind = BuildingKind::House});
    scene.world.commit();

    scene.run(kMigrantPeriodTicks);

    ASSERT_EQ(scene.walking(), 1);

    for (const auto walker :
         scene.world.view<Walker, antwika::game::Journey>())
    {
        EXPECT_EQ(
            scene.world.get<antwika::game::Journey>(walker).house,
            vacancy);
    }
}

TEST(PopulationSystemTest, Update_TurnsPeopleOutOnTheClockMigrantsArriveOn)
{
    Scene scene;
    scene.pave();
    scene.field[kAt] =
        requirementOf(HousingLevel::Hovel).desirability - 1;
    scene.settle(
        Household{.level = HousingLevel::Hovel, .population = 5});

    scene.run(1);

    EXPECT_EQ(scene.people(), 4);

    scene.run(kMigrantPeriodTicks - 1);

    EXPECT_EQ(scene.people(), 4);

    scene.run(1);

    EXPECT_EQ(scene.people(), 3);
}
