#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"

namespace
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::BuildTool;
    using antwika::game::Cell;
    using antwika::game::Footprint;
    using antwika::game::kSpawnPeriodTicks;
    using antwika::game::kWalkerLimit;
    using antwika::game::PathIndex;
    using antwika::game::spawnCellFor;
    using antwika::game::SpawnSystem;
    using antwika::game::Walker;
    using antwika::game::WalkerKind;
    using antwika::log::mocks::MockLogger;

    class SpawnSystemTest : public ::testing::Test
    {
    protected:
        Entity build(
            Cell at, BuildingKind kind = BuildingKind::Farm)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            return entity;
        }

        void pave(Cell at)
        {
            paths.insert(at);
        }

        void run(std::size_t ticks)
        {
            for (std::size_t tick = 0; tick < ticks; ++tick)
            {
                system.update(world, tick);
                world.commit();
            }
        }

        [[nodiscard]] std::size_t walkers()
        {
            return world.view<Walker, Cell>().size();
        }

        [[nodiscard]] std::vector<Cell> walkerCells()
        {
            std::vector<Cell> cells;
            for (const auto entity : world.view<Walker, Cell>())
            {
                cells.push_back(world.get<Cell>(entity));
            }
            return cells;
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        SpawnSystem system{paths, antwika::game::Tuning{}};
    };

    TEST_F(SpawnSystemTest, SpawnCellFor_TakesTheLowestNeighbouringRoad)
    {
        // Three of the four neighbours are paved.
        pave(Cell{.x = 5, .y = 4});
        pave(Cell{.x = 6, .y = 5});
        pave(Cell{.x = 4, .y = 5});

        const auto onto =
            spawnCellFor(Cell{.x = 5, .y = 5}, Footprint{}, paths);

        ASSERT_TRUE(onto.has_value());
        EXPECT_EQ(*onto, (Cell{.x = 4, .y = 5}));
    }

    TEST_F(SpawnSystemTest, SpawnCellFor_FindsNoneWithNoRoadBeside)
    {
        // Under it is not beside it.
        pave(Cell{.x = 5, .y = 5});

        EXPECT_FALSE(
            spawnCellFor(Cell{.x = 5, .y = 5}, Footprint{}, paths)
                .has_value());
    }

    TEST_F(SpawnSystemTest, Update_SendsNobodyOutBeforeTheIntervalIsUp)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks - 1);

        EXPECT_EQ(walkers(), 0U);
    }

    TEST_F(SpawnSystemTest, Update_SendsOneOutOnceTheIntervalIsUp)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);

        ASSERT_EQ(walkers(), 1U);
        EXPECT_EQ(walkerCells().front(), (Cell{.x = 5, .y = 6}));
    }

    // The rule the whole economy hangs off: one out at a time.
    TEST_F(SpawnSystemTest, Update_SendsNoSecondWalkerWhileTheFirstIsOut)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks * 3);

        EXPECT_EQ(walkers(), 1U);
    }

    TEST_F(SpawnSystemTest, Update_RemembersWhichWalkerItSentOut)
    {
        const auto building = build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);

        const auto sent = world.get<Building>(building).walkers[0];

        ASSERT_TRUE(world.alive(sent));
        EXPECT_TRUE(world.has<Walker>(sent));
    }

    TEST_F(SpawnSystemTest, Update_StampsAWalkerWithTheBuildingThatSentIt)
    {
        const auto building = build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);

        const auto sent = world.get<Building>(building).walkers[0];

        EXPECT_EQ(world.get<Walker>(sent).home, building);
    }

    TEST_F(SpawnSystemTest, Update_SendsAnotherOnceItsWalkerIsGone)
    {
        const auto building = build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);
        ASSERT_EQ(walkers(), 1U);

        world.destroy(world.get<Building>(building).walkers[0]);
        world.commit();

        run(kSpawnPeriodTicks);

        EXPECT_EQ(walkers(), 1U);
    }

    // A walker destroyed this tick still reads alive until the commit.
    // So its building is free from the next tick, not this one.
    // Worth pinning rather than leaving as folklore.
    TEST_F(SpawnSystemTest, Update_StillSeesItsWalkerOnTheTickItDies)
    {
        const auto building = build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);
        const auto sent = world.get<Building>(building).walkers[0];

        world.destroy(sent);
        system.update(world, 0);

        // Still the same walker: nothing new was staged this tick.
        EXPECT_EQ(world.get<Building>(building).walkers[0], sent);
    }

    // The test a tick-number modulus would fail.
    // Two houses a tick apart must keep their own rhythm.
    TEST_F(SpawnSystemTest, Update_GivesEachBuildingItsOwnCadence)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(1);

        build(Cell{.x = 8, .y = 8});
        pave(Cell{.x = 8, .y = 9});

        run(kSpawnPeriodTicks - 1);

        // The first is due and the second is one tick short of it.
        EXPECT_EQ(walkers(), 1U);

        run(1);

        EXPECT_EQ(walkers(), 2U);
    }

    TEST_F(SpawnSystemTest, Update_SendsNobodyOutOfAHouseWithNoRoadBeside)
    {
        build(Cell{.x = 5, .y = 5});

        run(kSpawnPeriodTicks * 3);

        EXPECT_EQ(walkers(), 0U);
    }

    // Held at zero rather than reset, so no debt accumulates.
    // One road, one walker -- not one per interval that went by.
    TEST_F(SpawnSystemTest, Update_OwesNothingForTheTicksItCouldNotSpawn)
    {
        build(Cell{.x = 5, .y = 5});

        run(kSpawnPeriodTicks * 5);

        pave(Cell{.x = 5, .y = 6});
        run(1);

        EXPECT_EQ(walkers(), 1U);
    }

    TEST_F(SpawnSystemTest, Update_SendsNobodyOutOfAHouse)
    {
        build(Cell{.x = 5, .y = 5}, BuildingKind::House);
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks * 2);

        EXPECT_EQ(walkers(), 0U);
    }

    TEST_F(SpawnSystemTest, Update_SendsThemOutOfEveryKindOfSource)
    {
        build(Cell{.x = 5, .y = 5}, BuildingKind::Well);
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);

        EXPECT_EQ(walkers(), 1U);
    }

    // An unbounded population is unbounded per-tick work.
    // One building cannot reach the cap on its own now.
    // So the crowd is seeded, and the cap stops the last one.
    TEST_F(SpawnSystemTest, Update_StopsAtTheWalkerLimit)
    {
        pave(Cell{.x = 0, .y = 1});

        for (std::size_t index = 0; index + 1 < kWalkerLimit; ++index)
        {
            const auto walker = world.create();
            world.add<Cell>(walker, Cell{.x = 0, .y = 1});
            world.add<Walker>(walker, Walker{});
        }

        build(Cell{.x = 0, .y = 0});
        build(Cell{.x = 2, .y = 0});
        pave(Cell{.x = 2, .y = 1});

        run(kSpawnPeriodTicks);

        EXPECT_EQ(walkers(), kWalkerLimit);
    }

    // One tick, several buildings, and the cap counts them all.
    TEST_F(SpawnSystemTest, Update_CountsWhatThisTickHasAlreadySpawned)
    {
        pave(Cell{.x = 0, .y = 1});

        for (std::size_t index = 0; index < kWalkerLimit + 2; ++index)
        {
            const auto walker = world.create();
            world.add<Cell>(walker, Cell{.x = 0, .y = 1});
            world.add<Walker>(walker, Walker{});
        }

        build(Cell{.x = 0, .y = 0});
        run(kSpawnPeriodTicks);

        EXPECT_EQ(walkers(), kWalkerLimit + 2);
    }

} // namespace

namespace
{
    // A fireman carries nothing.
    // So the load is the other arm of the one question asked.
    TEST_F(SpawnSystemTest, Update_SendsAFiremanOutCarryingNothing)
    {
        build(Cell{.x = 5, .y = 5}, BuildingKind::FireStation);
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);

        ASSERT_EQ(walkers(), 1U);

        for (const auto entity : world.view<Walker>())
        {
            EXPECT_EQ(world.get<Walker>(entity).kind, WalkerKind::Fireman);
            EXPECT_EQ(world.get<Walker>(entity).carried, 0);
        }
    }

    TEST_F(SpawnSystemTest, Update_SendsASellerOutLoaded)
    {
        build(Cell{.x = 5, .y = 5}, BuildingKind::Market);
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);

        ASSERT_EQ(walkers(), 1U);

        for (const auto entity : world.view<Walker>())
        {
            EXPECT_EQ(
                world.get<Walker>(entity).kind, WalkerKind::MarketSeller);
            EXPECT_EQ(
                world.get<Walker>(entity).carried,
                antwika::game::kWalkerLoad);
        }
    }

    // A cart pusher hauls whatever its errand names.
    // Rather than a load its kind decides, so it sets out empty.
    TEST_F(SpawnSystemTest, Update_SendsACartPusherOutEmpty)
    {
        build(Cell{.x = 5, .y = 5}, BuildingKind::Farm);
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);

        ASSERT_EQ(walkers(), 1U);

        for (const auto entity : world.view<Walker>())
        {
            EXPECT_EQ(world.get<Walker>(entity).kind, WalkerKind::CartPusher);
            EXPECT_EQ(world.get<Walker>(entity).carried, 0);
        }
    }
} // namespace

namespace
{
    // A road against a block's far corner is beside the building.
    // One cell's four neighbours would never have found it.
    TEST_F(SpawnSystemTest, SpawnCellFor_LooksRoundTheWholeBlock)
    {
        pave(Cell{.x = 6, .y = 5});

        EXPECT_FALSE(
            spawnCellFor(Cell{.x = 4, .y = 4}, Footprint{}, paths)
                .has_value());

        EXPECT_EQ(
            spawnCellFor(
                Cell{.x = 4, .y = 4}, Footprint{.width = 2, .height = 2},
                paths),
            (Cell{.x = 6, .y = 5}));
    }

    // Two roads against one block pick the same one every time.
    // Whichever order the perimeter walk found them.
    TEST_F(SpawnSystemTest, SpawnCellFor_StillTakesTheLowestOfTheBlock)
    {
        pave(Cell{.x = 6, .y = 5});
        pave(Cell{.x = 4, .y = 3});

        EXPECT_EQ(
            spawnCellFor(
                Cell{.x = 4, .y = 4}, Footprint{.width = 2, .height = 2},
                paths),
            (Cell{.x = 4, .y = 3}));
    }

    TEST_F(SpawnSystemTest, Update_SendsOneOutOfABlockOfMoreThanOneCell)
    {
        build(Cell{.x = 4, .y = 4}, BuildingKind::Farm);
        pave(Cell{.x = 6, .y = 5});

        run(kSpawnPeriodTicks);

        EXPECT_EQ(walkers(), 1U);
    }
    // A building holds kMaxWalkersOut handles.
    // A slot is capacity rather than leave to send another.
    TEST_F(SpawnSystemTest, Update_PutsItsWalkerInTheLowestFreeSlot)
    {
        const auto building = build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);

        const auto held = world.get<Building>(building).walkers;

        EXPECT_TRUE(world.alive(held[0]));
        EXPECT_EQ(held[1], antwika::ecs::kNullEntity);
    }

    // The rule that keeps a wider array from doubling a city.
    // The cadence sends one of its own kind at a time.
    TEST_F(SpawnSystemTest, Update_SendsNoSecondWalkerOfTheKindItSends)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks * 4);

        EXPECT_EQ(walkers(), 1U);
    }

    // Both slots taken is a building with nothing left to send into.
    TEST_F(SpawnSystemTest, Update_SendsNobodyWithEverySlotTaken)
    {
        const auto building = build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        // Two walkers of a kind this building does not send.
        // So only the slots can be what stops it.
        auto held = world.get<Building>(building);
        for (std::size_t slot = 0; slot < antwika::game::kMaxWalkersOut;
             ++slot)
        {
            const auto other = world.create();
            world.add<Cell>(other, Cell{.x = 1, .y = 1});
            world.add<Walker>(
                other, Walker{.kind = WalkerKind::MarketBuyer});
            held.walkers[slot] = other;
        }
        world.set<Building>(building, held);
        world.commit();

        run(kSpawnPeriodTicks * 2);

        EXPECT_EQ(walkers(), antwika::game::kMaxWalkersOut);
    }

    // freeWalkerSlot() reads alive() rather than kNullEntity.
    // So a handle whose walker has died frees its slot again.
    TEST_F(SpawnSystemTest, FreeWalkerSlot_ReadsAliveRatherThanTheHandle)
    {
        const auto building = build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kSpawnPeriodTicks);
        auto held = world.get<Building>(building);

        ASSERT_FALSE(
            antwika::game::freeWalkerSlot(world, held) == std::size_t{0});

        world.destroy(held.walkers[0]);
        world.commit();

        EXPECT_EQ(
            antwika::game::freeWalkerSlot(world, held), std::size_t{0});
    }

    // A slot may hold a handle whose Walker is not committed yet.
    // That is one somebody staged this very tick.
    // create() is immediate where add() is staged.
    // So alive() is true and has<Walker>() is not.
    // It counts, whatever kind it turns out to be.
    TEST_F(SpawnSystemTest, HasWalkerOfKind_CountsOneWithNoComponentYet)
    {
        Building held;
        held.walkers[0] = world.create();

        EXPECT_TRUE(
            antwika::game::hasWalkerOfKind(
                world, held, WalkerKind::CartPusher));
        EXPECT_TRUE(
            antwika::game::hasWalkerOfKind(
                world, held, WalkerKind::MarketSeller));
    }

    // And a slot holding somebody else's kind is not this cadence's.
    TEST_F(SpawnSystemTest, HasWalkerOfKind_IgnoresAWalkerOfAnotherKind)
    {
        Building held;
        const auto other = world.create();
        world.add<Walker>(other, Walker{.kind = WalkerKind::MarketBuyer});
        world.commit();
        held.walkers[0] = other;

        EXPECT_FALSE(
            antwika::game::hasWalkerOfKind(
                world, held, WalkerKind::CartPusher));
        EXPECT_TRUE(
            antwika::game::hasWalkerOfKind(
                world, held, WalkerKind::MarketBuyer));
    }

} // namespace

// The cap is the injected tuning's rather than the constant.
// A cap of nothing spawns nobody, however due a building is.
TEST_F(SpawnSystemTest, Update_HonoursTheConfiguredWalkerLimit)
{
    antwika::game::Tuning tuning;
    tuning.walkerLimit = 0;
    SpawnSystem tuned{paths, tuning};

    build(Cell{.x = 0, .y = 0});
    pave(Cell{.x = 0, .y = 1});

    for (std::size_t tick = 0; tick < kSpawnPeriodTicks; ++tick)
    {
        tuned.update(world, tick);
        world.commit();
    }

    EXPECT_EQ(walkers(), 0U);
}
