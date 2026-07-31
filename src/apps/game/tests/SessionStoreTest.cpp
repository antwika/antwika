#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/Walker.hpp"

namespace
{

    using antwika::ecs::World;
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::Direction;
    using antwika::game::GameState;
    using antwika::game::GridExtent;
    using antwika::game::Path;
    using antwika::game::PathIndex;
    using antwika::game::SaveGame;
    using antwika::game::SessionStore;
    using antwika::game::Walker;
    using antwika::game::WalkerView;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 16, .height = 16};

    class SessionStoreTest : public ::testing::Test
    {
    protected:
        void layPath(Cell cell)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
            paths.insert(cell);
        }

        [[nodiscard]] antwika::game::SceneSnapshot frame()
        {
            world.commit();
            return snapshotOf(world, paths, camera, kExtent);
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        Camera camera;
        GameState state;
        SessionStore store{world, paths, camera, state, kExtent, 42};
    };

    TEST_F(SessionStoreTest, Take_HoldsTheGridTheCameraAndTheState)
    {
        layPath(Cell{.x = 1, .y = 2});
        state.score = 7;
        camera.zoomIn();
        world.commit();

        const SaveGame save = store.take();

        EXPECT_EQ(save.paths, (std::vector<Cell>{{.x = 1, .y = 2}}));
        EXPECT_EQ(save.camera, camera);
        EXPECT_EQ(save.state.score, 7U);
        EXPECT_EQ(save.extent, kExtent);
        EXPECT_EQ(save.seed, 42U);
    }

    TEST_F(SessionStoreTest, Restore_PutsBackThePathsWalkersCameraAndState)
    {
        SaveGame save;
        save.paths = {{.x = 3, .y = 3}, {.x = 4, .y = 3}};
        save.walkers = {
            {.at = {.x = 3, .y = 3}, .facing = Direction::South}};
        save.camera = Camera(antwika::gfx::Point{.x = 9, .y = 9}, 1);
        save.state = GameState{.ticksProcessed = 5, .score = 3};

        store.restore(save);

        const auto after = frame();

        EXPECT_EQ(after.paths, save.paths);
        ASSERT_EQ(after.walkers.size(), 1U);
        EXPECT_EQ(after.walkers[0].at, (Cell{.x = 3, .y = 3}));
        EXPECT_EQ(after.walkers[0].facing, Direction::South);
        EXPECT_EQ(camera, save.camera);
        EXPECT_EQ(state, save.state);
        EXPECT_TRUE(paths.has(Cell{.x = 4, .y = 3}));
    }

    // Loading is resuming a session, not merging two.
    // A building the old city had must not still be standing.
    TEST_F(SessionStoreTest, Restore_DestroysWhatWasAlreadyOnTheGrid)
    {
        layPath(Cell{.x = 8, .y = 8});

        const auto house = world.create();
        world.add<Cell>(house, Cell{.x = 7, .y = 7});
        world.add<antwika::game::Building>(
            house,
            antwika::game::Building{
                .kind = antwika::game::BuildingKind::House});
        world.commit();

        SaveGame save;
        save.paths = {{.x = 1, .y = 1}};

        store.restore(save);

        const auto after = frame();

        EXPECT_EQ(after.paths, save.paths);
        EXPECT_FALSE(paths.has(Cell{.x = 8, .y = 8}));
        EXPECT_EQ(world.view<Path>().size(), 1U);
        EXPECT_TRUE(after.buildings.empty());
    }

    TEST_F(SessionStoreTest, Restore_ThenTake_ComesBackTheSame)
    {
        SaveGame save;
        save.paths = {{.x = 2, .y = 2}};
        save.walkers = {
            {.at = {.x = 2, .y = 2}, .facing = Direction::West}};
        save.camera = Camera(antwika::gfx::Point{.x = 4, .y = 5}, 2);
        save.state = GameState{.ticksProcessed = 9, .score = 1};
        save.extent = kExtent;
        save.seed = 42;

        store.restore(save);
        world.commit();

        EXPECT_EQ(store.take(), save);
    }

    // A fresh walker sets off at once, which is what a restored one is.
    TEST_F(SessionStoreTest, Restore_LeavesEveryWalkerReadyToStep)
    {
        SaveGame save;
        save.walkers = {
            {.at = {.x = 0, .y = 0}, .facing = Direction::East}};

        store.restore(save);
        world.commit();

        for (const auto entity : world.view<Walker>())
        {
            EXPECT_EQ(world.get<Walker>(entity).ticksUntilStep, 0U);
        }
    }

} // namespace
