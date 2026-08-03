#include "antwika/game/PopulationSystem.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WalkerSystem.hpp"

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
#include "antwika/game/GridExtent.hpp"
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
    using antwika::game::GridExtent;
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

    // One house, a road to the edge, a field, built fresh per case.
    //
    // The walkers run beside the system under test.
    // Nobody moves in without walking there any more.
    // A scene with no WalkerSystem would send for people.
    // Who would then stand on the gate for ever.
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

        // A road from the house's north side out to the west edge.
        // So there is a gate, and a walk of a few cells to make.
        void pave()
        {
            for (std::int32_t x = 0; x <= kAt.x; ++x)
            {
                paths.insert(Cell{.x = x, .y = kAt.y - 1});
            }
        }

        // A road beside the house that reaches no edge at all.
        void paveDeadEnd()
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
                walkers.update(world, static_cast<std::size_t>(tick));
                world.commit();
                system.update(world, static_cast<std::size_t>(tick));
                world.commit();
            }
        }

        // How many people the whole scene is carrying.
        // Both in the houses and on the roads between them.
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
        PopulationSystem system{paths, field, kExtent};
        antwika::game::WalkerSystem walkers{paths, kExtent};
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

// And a city walled off from the outside takes nobody in either.
// A road beside the house that reaches no edge is not a way in.
TEST(PopulationSystemTest, Update_HousesNobodyWithNoWayIntoTheCity)
{
    Scene scene;
    scene.paveDeadEnd();

    scene.run(4 * kSettlerPeriodTicks);

    EXPECT_EQ(scene.people(), 0);
    EXPECT_EQ(scene.walking(), 0);
}

// Sent for on the countdown, and counted when they get there.
// The walk is four cells at kTicksPerStep ticks each.
TEST(PopulationSystemTest, Update_MovesOnePersonInEachPeriod)
{
    Scene scene;
    scene.pave();

    scene.run(kSettlerPeriodTicks);

    // Sent for, and still on the road.
    EXPECT_EQ(scene.people(), 0);
    EXPECT_EQ(scene.walking(), 1);

    // The one sent for last period walks in during this one.
    // And the countdown sends for the next at the end of it.
    scene.run(kSettlerPeriodTicks);
    EXPECT_EQ(scene.people(), 1);

    scene.run(kSettlerPeriodTicks);
    EXPECT_EQ(scene.people(), 2);
}

// A migrant enters at the edge of the map rather than at the door.
TEST(PopulationSystemTest, Update_LetsAMigrantInAtTheEdgeOfTheMap)
{
    Scene scene;
    scene.pave();

    scene.run(kSettlerPeriodTicks);

    ASSERT_EQ(scene.walking(), 1);

    for (const auto entity : scene.world.view<Walker, Cell>())
    {
        // The west edge, which is the only gate this scene has.
        EXPECT_LE(scene.world.get<Cell>(entity).x, 1);
    }
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
    scene.run(kSettlerPeriodTicks * (capacity + 8));

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
    scene.run(kSettlerPeriodTicks * (capacity + 8));

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

    // And the person it could no longer hold is out on the road.
    // Rather than gone the instant the ceiling fell.
    EXPECT_EQ(scene.walking(), 1);
}

// A house holding more than its tier can turns the overflow out.
// With nowhere else in town to go, they walk to the edge and leave.
TEST(PopulationSystemTest, Update_WalksACrowdedHousesOverflowOffTheMap)
{
    Scene scene;
    scene.pave();

    const auto capacity = populationCapacityOf(HousingLevel::Tent);
    scene.settle(Household{.population = capacity + 2});

    scene.run(kSettlerPeriodTicks * 12);

    EXPECT_EQ(scene.people(), capacity);
    EXPECT_EQ(scene.walking(), 0);
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

// A city already at the walker cap sends for nobody.
// Which is the same backstop SpawnSystem and the markets keep.
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
    scene.run(kSettlerPeriodTicks * 2);

    EXPECT_EQ(scene.people(), 0);
}

// And a crowded one turns nobody out onto a full road either.
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
    scene.run(kSettlerPeriodTicks);

    // The person still leaves; only the walk is not drawn.
    EXPECT_EQ(scene.people(), over - 1);
    EXPECT_EQ(scene.walking(), 0);
}

// A block nothing reaches turns its overflow out with no walk at all.
TEST(PopulationSystemTest, Update_ThinsACrowdedHouseWithNoRoadBeside)
{
    Scene scene;

    const auto over = populationCapacityOf(HousingLevel::Tent) + 3;
    scene.settle(Household{.population = over});

    scene.run(kSettlerPeriodTicks);

    EXPECT_EQ(scene.people(), over - 1);
    EXPECT_EQ(scene.walking(), 0);
}

// A house shedding somebody with a neighbour that has room.
// They walk there rather than to the road out of town.
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

    scene.run(kSettlerPeriodTicks);

    // The one turned out is bound for the spare bed.
    // Rather than the road out of town, which turnOut() decides after.
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

// A migrant whose house was demolished mid-walk reaches nobody.
// Which is the arm WalkerSystem answers by retiring them.
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

// A house with both slots taken sends for nobody, however due it is.
// A slot is the whole of the "somebody is already coming" bookkeeping.
TEST(PopulationSystemTest, Update_SendsForNobodyWithBothSlotsTaken)
{
    Scene scene;
    scene.pave();

    auto building = scene.world.get<Building>(scene.house);

    for (std::size_t slot = 0; slot < antwika::game::kMaxWalkersOut;
         ++slot)
    {
        const auto held = scene.world.create();
        scene.world.add<Cell>(held, Cell{.x = 0, .y = kAt.y - 1});
        scene.world.add<Walker>(held, Walker{});
        building.walkers[slot] = held;
    }

    scene.world.set<Building>(scene.house, building);
    scene.world.commit();

    scene.run(kSettlerPeriodTicks * 2);

    EXPECT_EQ(scene.people(), 0);
}

// A migrant already on the road is what turns a second ask down.
// A slot holding one of some other kind is a different refusal.
TEST(PopulationSystemTest, Update_SendsForNobodyWithOneAlreadyWalking)
{
    Scene scene;
    scene.pave();

    const auto coming = scene.world.create();
    scene.world.add<Cell>(coming, Cell{.x = 0, .y = kAt.y - 1});
    scene.world.add<Walker>(
        coming,
        Walker{.kind = antwika::game::WalkerKind::Migrant});
    scene.world.add<antwika::game::Journey>(
        coming,
        antwika::game::Journey{.towards = kAt, .house = scene.house});
    scene.world.commit();

    auto building = scene.world.get<Building>(scene.house);
    building.walkers[0] = coming;
    scene.world.set<Building>(scene.house, building);
    scene.world.commit();

    // Due on the very next tick, while that one is still at the gate.
    scene.settle(Household{.ticksUntilSettler = 1});
    scene.run(1);

    // Still the one, rather than a second sent for on top of it.
    EXPECT_EQ(scene.walking(), 1);
    EXPECT_EQ(scene.people(), 0);
}

// A journey naming a house that has since been demolished.
// Answered by the world rather than by the stale cell it holds.
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

    // This system alone, since WalkerSystem would retire them first.
    // The two answer the same question and this is the one under test.
    scene.system.update(scene.world, 0);
    scene.world.commit();

    EXPECT_EQ(scene.people(), 0);
    EXPECT_TRUE(scene.world.alive(stray));
}

// A walk longer than the countdown that started it.
// The house asks again while its migrant is still on the road.
// The handle in its own slot is what turns the second ask down.
TEST(PopulationSystemTest, Update_AsksAgainForNobodyWhileOneIsWalking)
{
    Scene scene;
    scene.pave();

    scene.run(kSettlerPeriodTicks * 2 + 1);

    // One arrived and at most one more is on the road.
    // Never two of them, which is what a missing guard would send.
    EXPECT_LE(scene.walking(), 1);
}
