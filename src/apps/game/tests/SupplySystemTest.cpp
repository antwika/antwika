#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/SupplySystem.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Store.hpp"
#include "antwika/game/Walker.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingIndex;
    using antwika::game::BuildingKind;
    using antwika::game::capacityOf;
    using antwika::game::Cell;
    using antwika::game::Errand;
    using antwika::game::ErrandLeg;
    using antwika::game::footprintOf;
    using antwika::game::GridExtent;
    using antwika::game::kWalkerLimit;
    using antwika::game::kWalkerLoad;
    using antwika::game::SupplySystem;
    using antwika::game::PathIndex;
    using antwika::game::Resource;
    using antwika::game::resourceIndex;
    using antwika::game::Walker;
    using antwika::game::WalkerKind;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 14, .height = 14};

    class SupplySystemTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, BuildingKind kind, std::int32_t food)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(
                entity, Building{.kind = kind, .stock = {food, 0, 0}});
            world.commit();
            built.insert(at, footprintOf(kind));
            return entity;
        }

        Entity stock(Entity entity, Resource resource, std::int32_t held)
        {
            auto building = world.get<Building>(entity);
            building.stock[resourceIndex(resource)] = held;
            world.set<Building>(entity, building);
            world.commit();
            return entity;
        }

        Entity sendSeller(Cell at, Entity home)
        {
            const auto seller = world.create();
            world.add<Cell>(seller, at);
            world.add<Walker>(
                seller,
                Walker{
                    .kind = WalkerKind::MarketSeller,
                    .carried = kWalkerLoad,
                    .home = home});
            world.commit();

            auto sender = world.get<Building>(home);
            sender.walkers[1] = seller;
            world.set<Building>(home, sender);
            world.commit();
            return seller;
        }

        void paveRow(std::int32_t y, std::int32_t from, std::int32_t to)
        {
            for (std::int32_t x = from; x <= to; ++x)
            {
                paths.insert(Cell{.x = x, .y = y});
            }
        }

        void trade()
        {
            markets.update(world, 0);
            world.commit();
        }

        [[nodiscard]] std::vector<Entity> buyers()
        {
            std::vector<Entity> found;

            for (const auto entity : world.view<Walker, Cell>())
            {
                if (world.get<Walker>(entity).kind
                    == WalkerKind::MarketBuyer)
                {
                    found.push_back(entity);
                }
            }

            return found;
        }

        [[nodiscard]] std::int32_t food(Entity entity)
        {
            return world.get<Building>(entity)
                .stock[resourceIndex(Resource::Food)];
        }

        [[nodiscard]] std::int32_t held(Entity entity, Resource resource)
        {
            return world.get<Building>(entity)
                .stock[resourceIndex(resource)];
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;
        SupplySystem markets{paths, kExtent, antwika::game::GameConfig{}};
    };
}

TEST_F(SupplySystemTest, Update_PaysASellerOutOfTheMarketsOwnShelf)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 30);
    const auto seller = sendSeller(Cell{.x = 2, .y = 5}, market);

    trade();

    EXPECT_EQ(world.get<Walker>(seller).carried, 30);
    EXPECT_EQ(food(market), 0);
}

TEST_F(SupplySystemTest, Update_SendsASellerOutEmptyFromAnEmptyMarket)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto seller = sendSeller(Cell{.x = 2, .y = 5}, market);

    trade();

    EXPECT_EQ(world.get<Walker>(seller).carried, 0);
}

TEST_F(SupplySystemTest, Update_MarksASellerSoItIsNeverPaidTwice)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 100);
    const auto seller = sendSeller(Cell{.x = 2, .y = 5}, market);

    trade();
    const auto after = food(market);
    trade();

    EXPECT_EQ(food(market), after);
    EXPECT_TRUE(world.has<Errand>(seller));
    EXPECT_EQ(world.get<Errand>(seller).destination, kNullEntity);
}

TEST_F(SupplySystemTest, Update_SendsNoBuyerWithNothingToBuy)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 0);

    trade();

    EXPECT_TRUE(buyers().empty());
}

TEST_F(SupplySystemTest, Update_SendsABuyerToTheStoreWithTheGoods)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto sent = buyers();
    ASSERT_EQ(sent.size(), 1U);
    EXPECT_EQ(world.get<Cell>(sent[0]), (Cell{.x = 2, .y = 5}));
    EXPECT_EQ(world.get<Walker>(sent[0]).home, market);
    EXPECT_EQ(world.get<Errand>(sent[0]).destination, store);
    EXPECT_EQ(world.get<Errand>(sent[0]).leg, ErrandLeg::Outbound);
}

TEST_F(SupplySystemTest, Update_SendsOnlyOneBuyerAtATime)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();
    trade();

    EXPECT_EQ(buyers().size(), 1U);
}

TEST_F(SupplySystemTest, Update_SendsNoBuyerFromAMarketAlreadyFull)
{
    paveRow(5, 0, 13);
    build(
        Cell{.x = 2, .y = 3},
        BuildingKind::Market,
        capacityOf(BuildingKind::Market));
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    EXPECT_TRUE(buyers().empty());
}

TEST_F(SupplySystemTest, Update_SendsNoBuyerFromAMarketWithNoRoadBeside)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 0}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    EXPECT_TRUE(buyers().empty());
}

TEST_F(SupplySystemTest, Update_SendsNoBuyerFromAMarketWithNoFreeSlot)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    const auto first = sendSeller(Cell{.x = 2, .y = 5}, market);
    const auto second = sendSeller(Cell{.x = 3, .y = 5}, market);

    auto both = world.get<Building>(market);
    both.walkers[0] = first;
    both.walkers[1] = second;
    world.set<Building>(market, both);
    world.commit();

    trade();

    EXPECT_TRUE(buyers().empty());
}

TEST_F(SupplySystemTest, Update_LoadsABuyerStandingBesideItsStore)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    world.set<Cell>(buyer, Cell{.x = 8, .y = 5});
    world.commit();

    trade();

    EXPECT_EQ(world.get<Walker>(buyer).carried, 60);
    EXPECT_EQ(food(store), 0);
    EXPECT_EQ(world.get<Errand>(buyer).leg, ErrandLeg::Returning);
    EXPECT_EQ(food(market), 0);
}

TEST_F(SupplySystemTest, Update_LoadsNoMoreThanTheMarketHasRoomFor)
{
    paveRow(5, 0, 13);
    build(
        Cell{.x = 2, .y = 3},
        BuildingKind::Market,
        capacityOf(BuildingKind::Market) - 10);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    world.set<Cell>(buyer, Cell{.x = 8, .y = 5});
    world.commit();

    trade();

    EXPECT_EQ(world.get<Walker>(buyer).carried, 10);
    EXPECT_EQ(food(store), 50);
}

TEST_F(SupplySystemTest, Update_TurnsABuyerRoundAtAnEmptiedStore)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    world.set<Cell>(buyer, Cell{.x = 8, .y = 5});

    auto emptied = world.get<Building>(store);
    emptied.stock[resourceIndex(Resource::Food)] = 0;
    world.set<Building>(store, emptied);
    world.commit();

    trade();

    EXPECT_EQ(world.get<Walker>(buyer).carried, 0);
    EXPECT_EQ(world.get<Errand>(buyer).leg, ErrandLeg::Returning);
}

TEST_F(SupplySystemTest, Update_LeavesABuyerStillOnItsWayAlone)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    trade();

    EXPECT_EQ(world.get<Walker>(buyer).carried, 0);
    EXPECT_EQ(world.get<Errand>(buyer).leg, ErrandLeg::Outbound);
    EXPECT_EQ(food(store), 60);
}

TEST_F(SupplySystemTest, Update_LeavesABuyerWhoseStoreIsGoneToTheWalker)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    world.destroy(store);
    world.commit();

    trade();

    EXPECT_EQ(world.get<Errand>(buyer).leg, ErrandLeg::Outbound);
}

TEST_F(SupplySystemTest, Update_SplitsOneStoreBetweenTwoMarketsInCellOrder)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 10, .y = 3}, BuildingKind::Market, 0);
    const auto lower =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 40);

    trade();

    ASSERT_EQ(buyers().size(), 2U);

    for (const auto buyer : buyers())
    {
        const auto home = world.get<Walker>(buyer).home;
        world.set<Cell>(
            buyer,
            home == lower ? Cell{.x = 6, .y = 5} : Cell{.x = 7, .y = 5});
    }

    world.commit();
    trade();

    for (const auto buyer : buyers())
    {
        EXPECT_EQ(
            world.get<Walker>(buyer).carried,
            world.get<Walker>(buyer).home == lower ? 40 : 0);
    }
}

TEST_F(SupplySystemTest, Update_LeavesAWalkerThatIsNoMarketWalkerAlone)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 40);

    const auto fireman = world.create();
    world.add<Cell>(fireman, Cell{.x = 2, .y = 5});
    world.add<Walker>(
        fireman, Walker{.kind = WalkerKind::Fireman, .home = market});
    world.commit();

    auto sender = world.get<Building>(market);
    sender.walkers[1] = fireman;
    world.set<Building>(market, sender);
    world.commit();

    trade();

    EXPECT_FALSE(world.has<Errand>(fireman));
    EXPECT_EQ(food(market), 40);
}

TEST_F(SupplySystemTest, Update_CreditsTheMarketWhenItsBuyerGetsBack)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    world.set<Cell>(buyer, Cell{.x = 8, .y = 5});
    world.commit();

    trade();

    world.set<Cell>(buyer, Cell{.x = 2, .y = 5});
    world.commit();

    trade();

    EXPECT_EQ(food(market), 60);
    EXPECT_EQ(world.get<Walker>(buyer).carried, 0);
}

TEST_F(SupplySystemTest, Update_CreditsAMarketOnlyWhatItHasRoomFor)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    world.set<Cell>(buyer, Cell{.x = 8, .y = 5});
    world.commit();

    trade();

    auto stuffed = world.get<Building>(market);
    stuffed.stock[resourceIndex(Resource::Food)] =
        capacityOf(BuildingKind::Market) - 10;
    world.set<Building>(market, stuffed);
    world.set<Cell>(buyer, Cell{.x = 2, .y = 5});
    world.commit();

    trade();

    EXPECT_EQ(food(market), capacityOf(BuildingKind::Market));
    EXPECT_EQ(world.get<Walker>(buyer).carried, 50);
}

TEST_F(SupplySystemTest, Update_LeavesABuyerStillWalkingBackAlone)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    world.set<Cell>(buyer, Cell{.x = 8, .y = 5});
    world.commit();

    trade();

    world.set<Cell>(buyer, Cell{.x = 6, .y = 5});
    world.commit();

    trade();

    EXPECT_EQ(food(market), 0);
    EXPECT_EQ(world.get<Walker>(buyer).carried, 60);
}

TEST_F(SupplySystemTest, Update_LeavesAnEmptyBuyerComingHomeAlone)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    const auto buyer = buyers().front();
    world.set<Cell>(buyer, Cell{.x = 8, .y = 5});

    auto emptied = world.get<Building>(store);
    emptied.stock[resourceIndex(Resource::Food)] = 0;
    world.set<Building>(store, emptied);
    world.commit();

    trade();

    world.set<Cell>(buyer, Cell{.x = 2, .y = 5});
    world.commit();

    trade();

    EXPECT_EQ(food(market), 0);
}

TEST_F(SupplySystemTest, Update_LeavesABuyerWithNoErrandAlone)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    const auto stray = world.create();
    world.add<Cell>(stray, Cell{.x = 2, .y = 5});
    world.add<Walker>(
        stray,
        Walker{.kind = WalkerKind::MarketBuyer, .home = market});
    world.commit();

    auto held = world.get<Building>(market);
    held.walkers[1] = stray;
    world.set<Building>(market, held);
    world.commit();

    trade();

    EXPECT_FALSE(world.has<Errand>(stray));
}

TEST_F(SupplySystemTest, Update_SendsNoBuyerWithTheWalkerCapReached)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    for (std::size_t index = 0; index < kWalkerLimit;
         ++index)
    {
        const auto filler = world.create();
        world.add<Cell>(filler, Cell{.x = 0, .y = 5});
        world.add<Walker>(filler, Walker{});
    }

    world.commit();

    trade();

    EXPECT_TRUE(buyers().empty());
}

TEST_F(SupplySystemTest, Update_HandsEachSellerTheNextGoodInTurn)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, kWalkerLoad);
    stock(market, Resource::Pottery, kWalkerLoad);

    const auto second = sendSeller(Cell{.x = 3, .y = 5}, market);
    const auto first = sendSeller(Cell{.x = 2, .y = 5}, market);

    auto both = world.get<Building>(market);
    both.walkers[0] = first;
    both.walkers[1] = second;
    world.set<Building>(market, both);
    world.commit();

    trade();

    EXPECT_EQ(world.get<Errand>(first).carrying, Resource::Pottery);
    EXPECT_EQ(world.get<Errand>(second).carrying, Resource::Food);
    EXPECT_EQ(food(market), 0);
    EXPECT_EQ(held(market, Resource::Pottery), 0);
}

TEST_F(SupplySystemTest, Update_PassesOverAShelfWithNothingOnIt)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 40);
    const auto seller = sendSeller(Cell{.x = 2, .y = 5}, market);

    trade();

    EXPECT_EQ(world.get<Errand>(seller).carrying, Resource::Food);
    EXPECT_EQ(world.get<Walker>(seller).carried, 40);
}

TEST_F(SupplySystemTest, Update_BuysTheGoodTheMarketHasLeastOf)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 60);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);
    stock(store, Resource::Pottery, 60);

    trade();

    const auto sent = buyers();
    ASSERT_EQ(sent.size(), 1U);
    EXPECT_EQ(world.get<Errand>(sent[0]).carrying, Resource::Pottery);
}

TEST_F(SupplySystemTest, Update_CountsWhatItsSellerIsStillCarrying)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);
    stock(store, Resource::Pottery, 60);

    const auto seller = sendSeller(Cell{.x = 2, .y = 5}, market);
    world.add<Errand>(
        seller,
        Errand{
            .destination = kNullEntity,
            .carrying = Resource::Food,
            .leg = ErrandLeg::Outbound});
    world.commit();

    trade();

    const auto sent = buyers();
    ASSERT_EQ(sent.size(), 1U);
    EXPECT_EQ(world.get<Errand>(sent[0]).carrying, Resource::Pottery);
}

TEST_F(SupplySystemTest, Update_BuysWhatAStoreHasAndNotWhatItLacks)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 0);
    stock(store, Resource::Pottery, 60);

    trade();

    const auto sent = buyers();
    ASSERT_EQ(sent.size(), 1U);
    EXPECT_EQ(world.get<Errand>(sent[0]).carrying, Resource::Pottery);
}

TEST_F(SupplySystemTest, Update_ShelvesWhatItsBuyerBringsHome)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto store =
        build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 0);
    stock(store, Resource::Pottery, 60);

    trade();

    const auto buyer = buyers().front();
    world.set<Cell>(buyer, Cell{.x = 8, .y = 5});
    world.commit();

    trade();

    ASSERT_EQ(world.get<Walker>(buyer).carried, 60);
    EXPECT_EQ(held(store, Resource::Pottery), 0);

    world.set<Cell>(buyer, Cell{.x = 2, .y = 5});
    world.commit();

    trade();

    EXPECT_EQ(held(market, Resource::Pottery), 60);
    EXPECT_EQ(food(market), 0);
}
