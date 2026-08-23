#include <gtest/gtest.h>

#include <set>
#include <vector>

#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include <antwika/ecs/World.hpp>
#include <antwika/rules/Health.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/editor/editor/GameModule.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::voxel::VoxelPosition;
using antwika::voxel::Voxels;

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
        Voxels solidVoxels;
        std::vector<std::vector<VoxelPosition>> patrolPositions;
    };

}

TEST_F(GameModuleTest, GameModule_MakesAGameToBeReachedThrough)
{
    World world(logger);
    GameModule module(logger, world, solidVoxels, patrolPositions);

    EXPECT_TRUE(module->getWorld().isAlive(module->getEye()));
}

TEST_F(GameModuleTest, GameModule_IsReadThroughWhereItIsHeldAsConst)
{
    World world(logger);
    GameModule module(logger, world, solidVoxels, patrolPositions);
    const auto &constModule = module;

    EXPECT_EQ(constModule->getPlayer(), module->getPlayer());
}

TEST_F(GameModuleTest, GameModule_ClaimsThePlayForTheImageThatHoldsIt)
{
    World world(logger);
    GameModule module(logger, world, solidVoxels, patrolPositions);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Health>(entity, Health{.food = 7});
    }

    EXPECT_TRUE(world.has<Health>(entity));
}

#ifdef ANTWIKA_GAME_SHARED
TEST_F(GameModuleTest, Reload_LeavesWhatWasPlayedStandingInTheWorld)
{
    World world(logger);
    GameModule module(logger, world, solidVoxels, patrolPositions);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Health>(entity, Health{.food = 7});
        world.add<Inventory>(entity, Inventory{});
    }

    ASSERT_TRUE(module.reload());

    EXPECT_TRUE(world.isAlive(entity));
    ASSERT_TRUE(world.has<Health>(entity));
    EXPECT_EQ(world.get<Health>(entity).food, 7);
    EXPECT_TRUE(world.has<Inventory>(entity));
}

TEST_F(GameModuleTest, Reload_TakesUpTheEyeThatAlreadyStands)
{
    World world(logger);
    GameModule module(logger, world, solidVoxels, patrolPositions);
    const auto eye = module->eye();

    ASSERT_TRUE(module.reload());

    EXPECT_EQ(module->eye(), eye);
    EXPECT_TRUE(world.isAlive(eye));
}
#endif
