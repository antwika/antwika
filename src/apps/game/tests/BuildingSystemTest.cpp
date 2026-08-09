#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/BuildingSystem.hpp"
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
using antwika::game::Coverage;
using antwika::game::Errand;
using antwika::game::ErrandLeg;
using antwika::game::Footprint;
using antwika::game::footprintOf;
using antwika::game::GameConfig;
using antwika::game::kCoverageFull;
using antwika::game::kDrainPeriodTicks;
using antwika::game::kMaxRisk;
using antwika::game::kRiskPeriodTicks;
using antwika::game::kServiceCount;
using antwika::game::kStockCapacity;
using antwika::game::kWalkerLoad;
using antwika::game::Resource;
using antwika::game::resourceIndex;
using antwika::game::setCoverage;
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

        Entity sendSeller(
            Cell at,
            std::int32_t carried,
            Resource carrying = Resource::Food)
        {
            const auto seller = sendWalker(
                at,
                Walker{
                    .kind = WalkerKind::MarketSeller, .carried = carried});

            world.add<Errand>(
                seller,
                Errand{
                    .carrying = carrying, .leg = ErrandLeg::Outbound});
            world.commit();
            return seller;
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
            built,
            antwika::game::GridExtent{.width = 16, .height = 16},
            GameConfig{}};
    };
}

TEST_F(BuildingSystemTest, Update_HandsAWalkersLoadToTheBuildingBesideIt)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendSeller(Cell{.x = 0, .y = 0}, 40);

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 50);
}

TEST_F(BuildingSystemTest, Update_TakesWhatItGaveOffTheWalker)
{
    build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    const auto walker = sendSeller(Cell{.x = 0, .y = 0}, 40);

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

    const auto walker = sendSeller(Cell{.x = 0, .y = 0}, kWalkerLoad);

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        kStockCapacity);
    EXPECT_EQ(world.get<Walker>(walker).carried, kWalkerLoad - 5);
}

TEST_F(BuildingSystemTest, Update_AddsUpTwoDeliveriesInOneTick)
{
    const auto house = build(
        Cell{.x = 1, .y = 1},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendSeller(Cell{.x = 0, .y = 1}, 10);
    sendSeller(Cell{.x = 2, .y = 1}, 30);

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 50);
}

TEST_F(BuildingSystemTest, Update_HandsOverNothingToAShelfAlreadyFull)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .stock = {kStockCapacity, 0, 0}});

    const auto seller = sendSeller(Cell{.x = 0, .y = 0}, 40);

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        kStockCapacity);
    EXPECT_EQ(world.get<Walker>(seller).carried, 40);
}

TEST_F(BuildingSystemTest, Update_ShelvesWhatASellerCarriesAndNothingElse)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 0, 0}});

    sendSeller(Cell{.x = 0, .y = 0}, 40, Resource::Pottery);

    run(1);

    const auto stock = world.get<Building>(house).stock;

    EXPECT_EQ(stock[resourceIndex(Resource::Pottery)], 40);
    EXPECT_EQ(stock[resourceIndex(Resource::Food)], 10);
}

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

TEST_F(BuildingSystemTest, Update_ZeroesFireRiskForAPassingFireman)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .fireRisk = 60,
            .collapseRisk = 40});

    const auto fireman = sendWalker(
        Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Fireman});

    run(1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 0);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 40);
    EXPECT_EQ(world.get<Walker>(fireman).carried, 0);
}

TEST_F(BuildingSystemTest, Update_ZeroesCollapseRiskForAPassingEngineer)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .fireRisk = 60,
            .collapseRisk = 40});

    sendWalker(Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Engineer});

    run(1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 60);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 0);
}

TEST_F(BuildingSystemTest, Update_SavesABuildingWhoseRiskMaxesMidVisit)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .fireRisk = kMaxRisk,
            .ticksUntilRisk = 0});

    sendWalker(Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Fireman});

    run(1);

    EXPECT_TRUE(world.alive(house));
    EXPECT_EQ(world.get<Building>(house).fireRisk, 0);
}

TEST_F(BuildingSystemTest, Update_SavesABuildingFallingMidVisit)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .collapseRisk = kMaxRisk,
            .ticksUntilRisk = 0});

    sendWalker(Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Engineer});

    run(1);

    EXPECT_TRUE(world.alive(house));
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 0);
}

TEST_F(BuildingSystemTest, Update_LeavesABuildingAWalkerIsNotBesideAlone)
{
    const auto house = build(
        Cell{.x = 5, .y = 5},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendSeller(Cell{.x = 0, .y = 0}, 40);

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 10);
}

TEST_F(BuildingSystemTest, Update_DrainsAHouseOnItsOwnPeriod)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {50, 50, 50}});
    antwika::game::setHousehold(
        world,
        house,
        antwika::game::Household{.population = 1});
    world.commit();

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    const auto stock = world.get<Building>(house).stock;

    EXPECT_EQ(stock[resourceIndex(Resource::Food)], 49);
    EXPECT_EQ(stock[resourceIndex(Resource::Clay)], 49);
    EXPECT_EQ(stock[resourceIndex(Resource::Pottery)], 49);
}

TEST_F(BuildingSystemTest, Update_DrainsAHouseByItsOwnHeadcount)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {50, 50, 50}});
    antwika::game::setHousehold(
        world,
        house,
        antwika::game::Household{
            .population = antwika::game::kMouthsPerServing + 1});
    world.commit();

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        48);
}

TEST_F(BuildingSystemTest, Update_DrainsNothingFromAnEmptyHouse)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {50, 50, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        50);
}

TEST_F(BuildingSystemTest, Update_DrainsNothingFromAWorkshop)
{
    const auto workshop = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::Workshop, .stock = {0, 50, 0}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_EQ(
        world.get<Building>(workshop)
            .stock[resourceIndex(Resource::Clay)],
        50);
}

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
    const auto well = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::Well, .stock = {50, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_EQ(
        world.get<Building>(well).stock[resourceIndex(Resource::Food)], 50);
}

TEST_F(BuildingSystemTest, Update_RaisesEveryRiskWhereNobodyComes)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 1);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 1);
    EXPECT_EQ(world.get<Building>(house).diseaseRisk, 1);
}

TEST_F(BuildingSystemTest, Update_TakesDiseaseBackOffWhereMedicineIs)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .diseaseRisk = 40});
    cover(house, {0, kCoverageFull});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).diseaseRisk, 39);
}

TEST_F(BuildingSystemTest, Update_StepsDiseaseAgainstTheMedicineAlone)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});
    cover(house, {kCoverageFull, kCoverageFull});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 1);
    EXPECT_EQ(world.get<Building>(house).collapseRisk, 1);
    EXPECT_EQ(world.get<Building>(house).diseaseRisk, 0);
}

TEST_F(BuildingSystemTest, Update_NeverTakesDiseaseBelowNothing)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});
    cover(house, {0, kCoverageFull});

    run(3 * static_cast<std::size_t>(kRiskPeriodTicks));

    EXPECT_EQ(world.get<Building>(house).diseaseRisk, 0);
}

TEST_F(BuildingSystemTest, Update_NeverTakesDiseaseAboveTheMost)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .diseaseRisk = kMaxRisk,
            .ticksUntilRisk = 0});

    run(1);

    EXPECT_TRUE(world.alive(house));
    EXPECT_EQ(world.get<Building>(house).diseaseRisk, kMaxRisk);
}

TEST_F(BuildingSystemTest, Update_NeverTakesARiskAboveTheMost)
{
    const auto well = build(
        Cell{.x = 0, .y = 0},
        Building{
            .kind = BuildingKind::Well, .fireRisk = kMaxRisk - 1});

    run(2 * static_cast<std::size_t>(kRiskPeriodTicks));

    EXPECT_FALSE(world.alive(well));
}

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

TEST_F(BuildingSystemTest, Update_DemolishesABuildingThatRanOutOfLuck)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .fireRisk = kMaxRisk});

    run(1);

    EXPECT_FALSE(world.alive(house));
}

TEST_F(BuildingSystemTest, Update_KeepsAHouseThatHasRunOutOfFood)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {1, 50}});
    antwika::game::setHousehold(
        world,
        house,
        antwika::game::Household{.population = 1});
    world.commit();

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_TRUE(world.alive(house));
    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        0);
}

TEST_F(BuildingSystemTest, Update_FillsAGrownHousePastTheBottomShelf)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .stock = {kStockCapacity - 5, 0, 0}});
    antwika::game::setHousehold(
        world,
        house,
        antwika::game::Household{
            .level = antwika::game::HousingLevel::Cottage});
    world.commit();

    const auto seller = sendSeller(Cell{.x = 0, .y = 0}, kWalkerLoad);

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        kStockCapacity - 5 + kWalkerLoad);
    EXPECT_EQ(world.get<Walker>(seller).carried, 0);
}

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

TEST_F(BuildingSystemTest, Update_KeepsTheGroundOfAHouseHoldingNothing)
{
    build(
        Cell{.x = 4, .y = 4},
        Building{.kind = BuildingKind::House, .stock = {0, 0, 0}});

    ASSERT_TRUE(built.has(Cell{.x = 4, .y = 4}));

    run(1);

    EXPECT_TRUE(built.has(Cell{.x = 4, .y = 4}));
    EXPECT_EQ((world.view<antwika::game::Ruin, Cell>().size()), 0U);
}

TEST_F(BuildingSystemTest, Update_LeavesTheWalkerOfADemolishedBuilding)
{
    const auto walker = sendSeller(Cell{.x = 0, .y = 0}, kWalkerLoad);

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

TEST_F(BuildingSystemTest, Update_LeavesAStoreASellerWalksPastAlone)
{
    const auto store = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::Storage, .stock = {0, 0, 0}});

    const auto seller = sendSeller(Cell{.x = 0, .y = 0}, 40);

    run(1);

    EXPECT_EQ(
        world.get<Building>(store).stock[resourceIndex(Resource::Food)],
        0);
    EXPECT_EQ(world.get<Walker>(seller).carried, 40);
}

TEST_F(BuildingSystemTest, Update_LeavesTheMarketThatSentASellerAlone)
{
    const auto market = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::Market, .stock = {0, 0, 0}});

    const auto seller = sendSeller(Cell{.x = 0, .y = 0}, 40);

    auto sent = world.get<Walker>(seller);
    sent.home = market;
    world.set<Walker>(seller, sent);
    world.commit();

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

    EXPECT_TRUE(built.has(Cell{.x = 4, .y = 4}));
    EXPECT_TRUE(built.has(Cell{.x = 5, .y = 5}));

    const auto ruins = world.view<antwika::game::Ruin, Cell>();
    ASSERT_EQ(ruins.size(), 1U);
    EXPECT_EQ(
        world.get<antwika::game::Ruin>(*ruins.begin()).kind,
        BuildingKind::Farm);
}

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

TEST_F(BuildingSystemTest, Update_RelievesNothingAcrossOpenGround)
{
    const auto house = build(
        Cell{.x = 5, .y = 5},
        Building{.kind = BuildingKind::House, .fireRisk = 60});

    sendWalker(Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Fireman});

    run(1);

    EXPECT_EQ(world.get<Building>(house).fireRisk, 60);
}

TEST_F(BuildingSystemTest, Update_ResetsTheDrainToTheConfiguredPeriod)
{
    GameConfig config;
    config.drainPeriodTicks = 7;
    BuildingSystem tuned{
        built,
        antwika::game::GridExtent{.width = 16, .height = 16},
        config};

    const auto house = build(
        Cell{.x = 5, .y = 5},
        Building{.kind = BuildingKind::House, .ticksUntilDrain = 0});

    tuned.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<Building>(house).ticksUntilDrain, 7);
}
