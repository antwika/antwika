#include <gtest/gtest.h>

#include <set>
#include <vector>

#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include <antwika/ecs/World.hpp>
#include <antwika/rules/Health.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/editor/editor/GameModule.hpp"

using antwika::ecs::World;

using antwika::editor::GameModule;
using antwika::component::Health;
using antwika::component::Inventory;
using antwika::log::mocks::MockLogger;
using antwika::voxel::VoxelCell;
using ::testing::NiceMock;

namespace
{

    class GameModuleTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        std::set<VoxelCell> solidCells;
        std::vector<std::vector<VoxelCell>> patrolCells;
    };

}

TEST_F(GameModuleTest, GameModule_MakesAGameToBeReachedThrough)
{
    World world(logger);
    GameModule module(logger, world, solidCells, patrolCells);

    EXPECT_TRUE(module->world().alive(module->eye()));
}

TEST_F(GameModuleTest, GameModule_IsReadThroughWhereItIsHeldAsConst)
{
    World world(logger);
    GameModule module(logger, world, solidCells, patrolCells);
    const auto &constModule = module;

    EXPECT_EQ(constModule->player(), module->player());
}

TEST_F(GameModuleTest, GameModule_ClaimsThePlayForTheImageThatHoldsIt)
{
    World world(logger);
    GameModule module(logger, world, solidCells, patrolCells);
    const auto entity = world.create();

    world.add<Health>(entity, Health{.food = 7});
    world.commit();

    EXPECT_TRUE(world.has<Health>(entity));
}

#ifdef ANTWIKA_GAME_SHARED
TEST_F(GameModuleTest, Reload_LeavesWhatWasPlayedStandingInTheWorld)
{
    World world(logger);
    GameModule module(logger, world, solidCells, patrolCells);
    const auto entity = world.create();

    world.add<Health>(entity, Health{.food = 7});
    world.add<Inventory>(entity, Inventory{});
    world.commit();

    ASSERT_TRUE(module.reload());

    EXPECT_TRUE(world.alive(entity));
    ASSERT_TRUE(world.has<Health>(entity));
    EXPECT_EQ(world.get<Health>(entity).food, 7);
    EXPECT_TRUE(world.has<Inventory>(entity));
}

TEST_F(GameModuleTest, Reload_TakesUpTheEyeThatAlreadyStands)
{
    World world(logger);
    GameModule module(logger, world, solidCells, patrolCells);
    const auto eye = module->eye();

    ASSERT_TRUE(module.reload());

    EXPECT_EQ(module->eye(), eye);
    EXPECT_TRUE(world.alive(eye));
}
#endif
