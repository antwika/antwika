#include "antwika/game/MarketSystem.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

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
    using antwika::game::MarketSystem;
    using antwika::game::PathIndex;
    using antwika::game::Resource;
    using antwika::game::resourceIndex;
    using antwika::game::Walker;
    using antwika::game::WalkerKind;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 14, .height = 14};

    class MarketSystemTest : public ::testing::Test
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

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;
        MarketSystem markets{paths, kExtent};
    };
} // namespace

// The bug this whole system exists to close.
// SpawnSystem sends the seller out with the load its kind carries.
// Here it is paid for out of what the market actually holds.
TEST_F(MarketSystemTest, Update_PaysASellerOutOfTheMarketsOwnShelf)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 30);
    const auto seller = sendSeller(Cell{.x = 2, .y = 5}, market);

    trade();

    EXPECT_EQ(world.get<Walker>(seller).carried, 30);
    EXPECT_EQ(food(market), 0);
}

TEST_F(MarketSystemTest, Update_SendsASellerOutEmptyFromAnEmptyMarket)
{
    paveRow(5, 0, 13);
    const auto market =
        build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    const auto seller = sendSeller(Cell{.x = 2, .y = 5}, market);

    trade();

    EXPECT_EQ(world.get<Walker>(seller).carried, 0);
}

TEST_F(MarketSystemTest, Update_MarksASellerSoItIsNeverPaidTwice)
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

TEST_F(MarketSystemTest, Update_SendsNoBuyerWithNothingToBuy)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 0);

    trade();

    EXPECT_TRUE(buyers().empty());
}

TEST_F(MarketSystemTest, Update_SendsABuyerToTheStoreWithTheGoods)
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

TEST_F(MarketSystemTest, Update_SendsOnlyOneBuyerAtATime)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 3}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();
    trade();

    EXPECT_EQ(buyers().size(), 1U);
}

TEST_F(MarketSystemTest, Update_SendsNoBuyerFromAMarketAlreadyFull)
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

TEST_F(MarketSystemTest, Update_SendsNoBuyerFromAMarketWithNoRoadBeside)
{
    paveRow(5, 0, 13);
    build(Cell{.x = 2, .y = 0}, BuildingKind::Market, 0);
    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 60);

    trade();

    EXPECT_TRUE(buyers().empty());
}

TEST_F(MarketSystemTest, Update_SendsNoBuyerFromAMarketWithNoFreeSlot)
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

TEST_F(MarketSystemTest, Update_LoadsABuyerStandingBesideItsStore)
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

TEST_F(MarketSystemTest, Update_LoadsNoMoreThanTheMarketHasRoomFor)
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

// A store emptied while the buyer walked to it turns it round.
// Rather than leaving it standing at the door for ever.
TEST_F(MarketSystemTest, Update_TurnsABuyerRoundAtAnEmptiedStore)
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

TEST_F(MarketSystemTest, Update_LeavesABuyerStillOnItsWayAlone)
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

TEST_F(MarketSystemTest, Update_LeavesABuyerWhoseStoreIsGoneToTheWalker)
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

// Two markets, one storehouse, and one load between them.
// The lower market gets it, whichever order they were built in.
TEST_F(MarketSystemTest, Update_SplitsOneStoreBetweenTwoMarketsInCellOrder)
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

TEST_F(MarketSystemTest, Update_LeavesAWalkerThatIsNoMarketWalkerAlone)
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

// The other half of the buyer's round trip.
// Here rather than in BuildingSystem: this phase is the market's own.
TEST_F(MarketSystemTest, Update_CreditsTheMarketWhenItsBuyerGetsBack)
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

TEST_F(MarketSystemTest, Update_CreditsAMarketOnlyWhatItHasRoomFor)
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

TEST_F(MarketSystemTest, Update_LeavesABuyerStillWalkingBackAlone)
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

TEST_F(MarketSystemTest, Update_LeavesAnEmptyBuyerComingHomeAlone)
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

// A file may hold a market buyer with no errand at all.
// Which is a walker that roams, and nothing this system acts on.
TEST_F(MarketSystemTest, Update_LeavesABuyerWithNoErrandAlone)
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

TEST_F(MarketSystemTest, Update_SendsNoBuyerWithTheWalkerCapReached)
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
