#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"

namespace
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildTool;
    using antwika::game::Cell;
    using antwika::game::kTicksPerSpawn;
    using antwika::game::kWalkerLimit;
    using antwika::game::PathIndex;
    using antwika::game::spawnCellFor;
    using antwika::game::SpawnSystem;
    using antwika::game::Walker;
    using antwika::log::mocks::MockLogger;

    class SpawnSystemTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, BuildTool kind = BuildTool::House)
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
        SpawnSystem system{paths};
    };

    TEST_F(SpawnSystemTest, SpawnCellFor_TakesTheLowestNeighbouringRoad)
    {
        // Three of the four neighbours are paved.
        pave(Cell{.x = 5, .y = 4});
        pave(Cell{.x = 6, .y = 5});
        pave(Cell{.x = 4, .y = 5});

        const auto onto = spawnCellFor(Cell{.x = 5, .y = 5}, paths);

        ASSERT_TRUE(onto.has_value());
        EXPECT_EQ(*onto, (Cell{.x = 4, .y = 5}));
    }

    TEST_F(SpawnSystemTest, SpawnCellFor_FindsNoneWithNoRoadBeside)
    {
        // Under it is not beside it.
        pave(Cell{.x = 5, .y = 5});

        EXPECT_FALSE(spawnCellFor(Cell{.x = 5, .y = 5}, paths).has_value());
    }

    TEST_F(SpawnSystemTest, Update_SendsNobodyOutBeforeTheIntervalIsUp)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kTicksPerSpawn - 1);

        EXPECT_EQ(walkers(), 0U);
    }

    TEST_F(SpawnSystemTest, Update_SendsOneOutOnceTheIntervalIsUp)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kTicksPerSpawn);

        ASSERT_EQ(walkers(), 1U);
        EXPECT_EQ(walkerCells().front(), (Cell{.x = 5, .y = 6}));
    }

    TEST_F(SpawnSystemTest, Update_KeepsSendingThemOutOnTheSameCadence)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kTicksPerSpawn * 3);

        EXPECT_EQ(walkers(), 3U);
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

        run(kTicksPerSpawn - 1);

        // The first is due and the second is one tick short of it.
        EXPECT_EQ(walkers(), 1U);

        run(1);

        EXPECT_EQ(walkers(), 2U);
    }

    TEST_F(SpawnSystemTest, Update_SendsNobodyOutOfAHouseWithNoRoadBeside)
    {
        build(Cell{.x = 5, .y = 5});

        run(kTicksPerSpawn * 3);

        EXPECT_EQ(walkers(), 0U);
    }

    // Held at zero rather than reset, so no debt accumulates.
    // One road, one walker -- not one per interval that went by.
    TEST_F(SpawnSystemTest, Update_OwesNothingForTheTicksItCouldNotSpawn)
    {
        build(Cell{.x = 5, .y = 5});

        run(kTicksPerSpawn * 5);

        pave(Cell{.x = 5, .y = 6});
        run(1);

        EXPECT_EQ(walkers(), 1U);
    }

    TEST_F(SpawnSystemTest, Update_SendsNobodyOutOfATower)
    {
        build(Cell{.x = 5, .y = 5}, BuildTool::Tower);
        pave(Cell{.x = 5, .y = 6});

        run(kTicksPerSpawn * 2);

        EXPECT_EQ(walkers(), 0U);
    }

    TEST_F(SpawnSystemTest, Update_SendsThemOutOfAShopToo)
    {
        build(Cell{.x = 5, .y = 5}, BuildTool::Shop);
        pave(Cell{.x = 5, .y = 6});

        run(kTicksPerSpawn);

        EXPECT_EQ(walkers(), 1U);
    }

    // An unbounded population is unbounded per-tick work.
    TEST_F(SpawnSystemTest, Update_StopsAtTheWalkerLimit)
    {
        build(Cell{.x = 5, .y = 5});
        pave(Cell{.x = 5, .y = 6});

        run(kTicksPerSpawn * (kWalkerLimit + 4));

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
        run(kTicksPerSpawn);

        EXPECT_EQ(walkers(), kWalkerLimit + 2);
    }

} // namespace
