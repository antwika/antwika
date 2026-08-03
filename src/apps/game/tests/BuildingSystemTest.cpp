#include "antwika/game/BuildingSystem.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingIndex;
using antwika::game::BuildingKind;
using antwika::game::BuildingSystem;
using antwika::game::Cell;
using antwika::game::Errand;
using antwika::game::ErrandLeg;
using antwika::game::Footprint;
using antwika::game::footprintOf;
using antwika::game::Coverage;
using antwika::game::kCoverageFull;
using antwika::game::kDrainPeriodTicks;
using antwika::game::kMaxRisk;
using antwika::game::kRiskPeriodTicks;
using antwika::game::kServiceCount;
using antwika::game::kStockCapacity;
using antwika::game::setCoverage;
using antwika::game::kWalkerLoad;
using antwika::game::Resource;
using antwika::game::resourceIndex;
using antwika::game::Walker;
using antwika::game::WalkerKind;
using antwika::log::mocks::MockLogger;

namespace
{
    class BuildingSystemTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, Building building)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, building);
            world.commit();
            built.insert(at, footprintOf(building.kind));
            return entity;
        }

        void cover(
            Entity entity,
            std::array<std::int32_t, kServiceCount> ticksLeft)
        {
            setCoverage(world, entity, Coverage{.ticksLeft = ticksLeft});
            world.commit();
        }

        Entity sendWalker(Cell at, Walker walker)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(entity, walker);
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

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        BuildingIndex built;
        BuildingSystem system{
            built, antwika::game::GridExtent{.width = 16, .height = 16}};
    };
} // namespace

TEST_F(BuildingSystemTest, Update_HandsAWalkersLoadToTheBuildingBesideIt)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 50);
}

TEST_F(BuildingSystemTest, Update_TakesWhatItGaveOffTheWalker)
{
    build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    const auto walker = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

    run(1);

    EXPECT_EQ(world.get<Walker>(walker).carried, 0);
}

TEST_F(BuildingSystemTest, Update_DeliversOnlyWhatTheBuildingHasRoomFor)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .stock = {kStockCapacity - 5, 10}});

    const auto walker = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = kWalkerLoad});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        kStockCapacity);
    EXPECT_EQ(world.get<Walker>(walker).carried, kWalkerLoad - 5);
}

// The double-buffer trap.
// Both walkers read the same committed amount.
// So a plain write would let the second overwrite the first.
TEST_F(BuildingSystemTest, Update_AddsUpTwoDeliveriesInOneTick)
{
    const auto house = build(
        Cell{.x = 1, .y = 1},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 0, .y = 1},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 10});
    sendWalker(
        Cell{.x = 2, .y = 1},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 30});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 50);
}

// A shelf already at capacity has no room for anything.
// So nothing changes hands and the walker keeps its load.
TEST_F(BuildingSystemTest, Update_HandsOverNothingToAShelfAlreadyFull)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .stock = {kStockCapacity, 0, 0}});

    const auto seller = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        kStockCapacity);
    EXPECT_EQ(world.get<Walker>(seller).carried, 40);
}

// A water carrier hands nothing over: water is a service now.
// So what it leaves behind is a shelf exactly where it was.
TEST_F(BuildingSystemTest, Update_LeavesEveryShelfAloneForAServiceWalker)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 20, 30}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::WaterCarrier, .carried = 20});

    run(1);

    const auto stock = world.get<Building>(house).stock;

    EXPECT_EQ(stock[resourceIndex(Resource::Food)], 10);
    EXPECT_EQ(stock[resourceIndex(Resource::Clay)], 20);
    EXPECT_EQ(stock[resourceIndex(Resource::Pottery)], 30);
}

// A fireman used to take a fixed amount of risk off here.
// He now refreshes coverage instead -- see CoverageSystem.
// So a delivery has nothing whatever to do with one.
TEST_F(BuildingSystemTest, Update_LeavesRiskAloneForAPassingFireman)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .fireRisk = 60});

    const auto fireman = sendWalker(
        Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Fireman});

    run(1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 60);
    EXPECT_EQ(world.get<Walker>(fireman).carried, 0);
}

TEST_F(BuildingSystemTest, Update_LeavesABuildingAWalkerIsNotBesideAlone)
{
    const auto house = build(
        Cell{.x = 5, .y = 5},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 10);
}

TEST_F(BuildingSystemTest, Update_DrainsAHouseOnItsOwnPeriod)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {50, 50, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    const auto stock = world.get<Building>(house).stock;

    EXPECT_EQ(stock[resourceIndex(Resource::Food)], 49);
    EXPECT_EQ(stock[resourceIndex(Resource::Clay)], 49);
    EXPECT_EQ(stock[resourceIndex(Resource::Pottery)], 49);
}

// Only what sustains() names is a larder.
// A house with no pottery is a house nobody has sold any to yet.
TEST_F(BuildingSystemTest, Update_KeepsAHouseThatHasRunOutOfAComfort)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {50, 0, 0}});

    run(1);

    EXPECT_TRUE(world.alive(house));
}

TEST_F(BuildingSystemTest, Update_LeavesASourcesStockWhereItIs)
{
    // A source is not a place anybody eats.
    const auto well = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::Well, .stock = {50, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_EQ(
        world.get<Building>(well).stock[resourceIndex(Resource::Food)], 50);
}

// A district nobody serves is a district in both kinds of danger.
TEST_F(BuildingSystemTest, Update_RaisesBothRisksWithNoCoverageAtAll)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 1);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 1);
}

// And one that both a fireman and an engineer reach works both off.
TEST_F(BuildingSystemTest, Update_TakesRiskBackOffWhereBothServicesReach)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .fireRisk = 40,
            .collapseRisk = 30});
    cover(house, {0, 0, kCoverageFull, kCoverageFull});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 39);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 29);
}

// The split's whole point: each risk answers to its own service.
// A fireman's rounds say nothing about the roof falling in.
TEST_F(BuildingSystemTest, Update_StepsEachRiskAgainstItsOwnService)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .fireRisk = 40});
    cover(house, {0, 0, kCoverageFull, 0});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 39);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 1);
}

TEST_F(BuildingSystemTest, Update_NeverTakesARiskBelowNothing)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});
    cover(house, {0, 0, kCoverageFull, kCoverageFull});

    run(3 * static_cast<std::size_t>(kRiskPeriodTicks));

    EXPECT_EQ(world.get<Building>(house).fireRisk, 0);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 0);
}

TEST_F(BuildingSystemTest, Update_NeverTakesARiskAboveTheMost)
{
    // A source, so an empty larder is not what ends it.
    const auto well = build(
        Cell{.x = 0, .y = 0},
        Building{
            .kind = BuildingKind::Well, .fireRisk = kMaxRisk - 1});

    run(2 * static_cast<std::size_t>(kRiskPeriodTicks));

    EXPECT_FALSE(world.alive(well));
}

// A countdown that has not run out is a countdown and nothing else.
TEST_F(BuildingSystemTest, Update_LeavesRiskAloneBeforeItsPeriodIsUp)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .fireRisk = 5,
            .collapseRisk = 7});

    run(static_cast<std::size_t>(kRiskPeriodTicks) - 1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 5);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 7);
    EXPECT_EQ(world.get<Building>(house).ticksUntilRisk, 1);
}

// What is still this system's is what a building at the most is for.
TEST_F(BuildingSystemTest, Update_DemolishesABuildingThatRanOutOfLuck)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .fireRisk = kMaxRisk});

    run(1);

    EXPECT_FALSE(world.alive(house));
}

TEST_F(BuildingSystemTest, Update_DemolishesAHouseThatRanOutOfFood)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {1, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_FALSE(world.alive(house));
}

// Fire is not bare ground: the block stays claimed.
// Only the raze tool ever frees what a fire took.
TEST_F(BuildingSystemTest, Update_KeepsTheGroundOfWhatCatchesFire)
{
    build(
        Cell{.x = 4, .y = 4},
        Building{.kind = BuildingKind::House, .fireRisk = kMaxRisk});

    ASSERT_TRUE(built.has(Cell{.x = 4, .y = 4}));

    run(1);

    EXPECT_TRUE(built.has(Cell{.x = 4, .y = 4}));

    const auto ruins = world.view<antwika::game::Ruin, Cell>();
    ASSERT_EQ(ruins.size(), 1U);

    const auto ruin =
        world.get<antwika::game::Ruin>(*ruins.begin());
    EXPECT_EQ(ruin.kind, BuildingKind::House);
    EXPECT_EQ(ruin.state, antwika::game::RuinState::Burning);
    EXPECT_EQ(
        ruin.ticksUntilOut, antwika::game::kBurnDurationTicks);
    EXPECT_EQ(
        world.get<Cell>(*ruins.begin()), (Cell{.x = 4, .y = 4}));
}

// Starving is still the ending that frees the ground.
// A house lost to hunger fell down rather than burnt.
TEST_F(BuildingSystemTest, Update_ClearsTheCellOfWhatItStarves)
{
    build(
        Cell{.x = 4, .y = 4},
        Building{.kind = BuildingKind::House, .stock = {0, 0, 0}});

    ASSERT_TRUE(built.has(Cell{.x = 4, .y = 4}));

    run(1);

    EXPECT_FALSE(built.has(Cell{.x = 4, .y = 4}));
    EXPECT_EQ((world.view<antwika::game::Ruin, Cell>().size()), 0U);
}

TEST_F(BuildingSystemTest, Update_LeavesTheWalkerOfADemolishedBuilding)
{
    // The walker outlives its home.
    // WalkerSystem removes it once its own budget runs out.
    const auto walker = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = kWalkerLoad});

    build(
        Cell{.x = 5, .y = 5},
        Building{.kind = BuildingKind::House, .fireRisk = kMaxRisk});

    run(1);

    EXPECT_TRUE(world.alive(walker));
}

TEST_F(BuildingSystemTest, Update_DoesNothingWithNoBuildingsAtAll)
{
    EXPECT_NO_THROW(run(1));
}

TEST_F(BuildingSystemTest, Update_ReachesABuildingByAnyCellOfItsBlock)
{
    // Beside the block's far corner rather than its origin.
    // One cell's neighbours would never have matched it.
    //
    // Asked with a cart on an errand rather than with a loose load.
    // A load bound nowhere only ever reaches somebody who eats it.
    // And every kind that eats stands on one cell.
    const auto store = build(
        Cell{.x = 4, .y = 4},
        Building{.kind = BuildingKind::Storage, .stock = {10, 0, 0}});

    const auto cart = sendWalker(
        Cell{.x = 7, .y = 5},
        Walker{.kind = WalkerKind::CartPusher, .carried = 20});

    world.add<Errand>(
        cart,
        Errand{
            .destination = store,
            .carrying = Resource::Food,
            .leg = ErrandLeg::Outbound});
    world.commit();

    run(1);

    EXPECT_EQ(
        world.get<Building>(store).stock[resourceIndex(Resource::Food)],
        30);
}

// A seller filling the storehouse it passed would undo the buyer.
// So a load bound nowhere goes to whoever eats it and nobody else.
TEST_F(BuildingSystemTest, Update_LeavesAStoreASellerWalksPastAlone)
{
    const auto store = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::Storage, .stock = {0, 0, 0}});

    const auto seller = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

    run(1);

    EXPECT_EQ(
        world.get<Building>(store).stock[resourceIndex(Resource::Food)],
        0);
    EXPECT_EQ(world.get<Walker>(seller).carried, 40);
}

// Handing a basket back to the building that filled it moves nothing.
TEST_F(BuildingSystemTest, Update_LeavesTheMarketThatSentASellerAlone)
{
    const auto market = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::Market, .stock = {0, 0, 0}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{
            .kind = WalkerKind::MarketSeller,
            .carried = 40,
            .home = market});

    run(1);

    EXPECT_EQ(
        world.get<Building>(market).stock[resourceIndex(Resource::Food)],
        0);
}

TEST_F(BuildingSystemTest, Update_KeepsEveryCellOfABurningBlock)
{
    build(
        Cell{.x = 4, .y = 4},
        Building{.kind = BuildingKind::Farm, .fireRisk = kMaxRisk});

    ASSERT_TRUE(built.has(Cell{.x = 5, .y = 5}));

    run(1);

    // The whole block is still claimed.
    // And the ruin remembers the kind.
    // Which is what names the block's size from now on.
    EXPECT_TRUE(built.has(Cell{.x = 4, .y = 4}));
    EXPECT_TRUE(built.has(Cell{.x = 5, .y = 5}));

    const auto ruins = world.view<antwika::game::Ruin, Cell>();
    ASSERT_EQ(ruins.size(), 1U);
    EXPECT_EQ(
        world.get<antwika::game::Ruin>(*ruins.begin()).kind,
        BuildingKind::Farm);
}

// A load that never changed hands belongs to whoever sent the walker.
// Not to this system -- see acceptsAt().
TEST_F(BuildingSystemTest, Update_LeavesAWalkerOnItsWayBackToItsSender)
{
    const auto market = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::Market, .stock = {0, 0, 0}});

    const auto buyer = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{
            .kind = WalkerKind::MarketBuyer,
            .carried = 40,
            .home = market});

    world.add<Errand>(
        buyer,
        Errand{
            .carrying = Resource::Food, .leg = ErrandLeg::Returning});
    world.commit();

    run(1);

    EXPECT_EQ(
        world.get<Building>(market).stock[resourceIndex(Resource::Food)],
        0);
    EXPECT_EQ(world.get<Walker>(buyer).carried, 40);
}

// A building lost to the economy goes through the same demolish().
// So its people walk out exactly as a razed one's do.
TEST_F(BuildingSystemTest, Update_TurnsTheOccupantsOutOfWhatItLoses)
{
    const auto house = build(
        Cell{.x = 4, .y = 4},
        Building{.kind = BuildingKind::House, .fireRisk = kMaxRisk});
    antwika::game::setHousehold(
        world,
        house,
        antwika::game::Household{
            .level = antwika::game::HousingLevel::Tent,
            .population = 2});
    world.commit();

    run(1);

    EXPECT_FALSE(world.alive(house));
    EXPECT_EQ(
        (world.view<Walker, antwika::game::Journey>().size()), 2U);
}

// The collapse ending: straight to debris, and the ground is kept.
// The fire's ending without the fire, so nothing is left to put out.
TEST_F(BuildingSystemTest, Update_DropsACollapsedBuildingToDebris)
{
    build(
        Cell{.x = 4, .y = 4},
        Building{
            .kind = BuildingKind::House, .collapseRisk = kMaxRisk});

    run(1);

    EXPECT_TRUE(built.has(Cell{.x = 4, .y = 4}));

    const auto ruins = world.view<antwika::game::Ruin, Cell>();
    ASSERT_EQ(ruins.size(), 1U);

    const auto ruin = world.get<antwika::game::Ruin>(*ruins.begin());
    EXPECT_EQ(ruin.kind, BuildingKind::House);
    EXPECT_EQ(ruin.state, antwika::game::RuinState::Debris);
    EXPECT_EQ(ruin.ticksUntilOut, 0);
}

// Both risks maxed in one tick take the first ending asked.
// Which is fire, so what stands afterwards still burns.
TEST_F(BuildingSystemTest, Update_PrefersTheFireWhereBothRisksMax)
{
    build(
        Cell{.x = 4, .y = 4},
        Building{
            .kind = BuildingKind::House,
            .fireRisk = kMaxRisk,
            .collapseRisk = kMaxRisk});

    run(1);

    const auto ruins = world.view<antwika::game::Ruin, Cell>();
    ASSERT_EQ(ruins.size(), 1U);
    EXPECT_EQ(
        world.get<antwika::game::Ruin>(*ruins.begin()).state,
        antwika::game::RuinState::Burning);
}

// A collapse turns its people out exactly as a fire does.
TEST_F(BuildingSystemTest, Update_TurnsTheOccupantsOutOfWhatCollapses)
{
    const auto house = build(
        Cell{.x = 4, .y = 4},
        Building{
            .kind = BuildingKind::House, .collapseRisk = kMaxRisk});
    antwika::game::setHousehold(
        world,
        house,
        antwika::game::Household{
            .level = antwika::game::HousingLevel::Tent,
            .population = 2});
    world.commit();

    run(1);

    EXPECT_FALSE(world.alive(house));
    EXPECT_EQ(
        (world.view<Walker, antwika::game::Journey>().size()), 2U);
}
