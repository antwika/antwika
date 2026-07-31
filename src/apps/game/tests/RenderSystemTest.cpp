#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/TileAtlas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::ecs::World;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::MainMenuScene;
using antwika::game::PathIndex;
using antwika::game::RenderSetup;
using antwika::game::RenderSystem;
using antwika::game::roadTile;
using antwika::game::UiOverlay;
using antwika::game::WorldMapScene;
using antwika::game::WorldMapState;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx::mocks::MockWindow;
using antwika::log::mocks::MockLogger;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr Size kCanvas{.width = 320, .height = 240};
    constexpr GridExtent kExtent{.width = 2, .height = 2};

    class RenderSystemTest : public ::testing::Test
    {
    protected:
        RenderSystemTest()
        {
            ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
            ON_CALL(window, size()).WillByDefault(Return(kCanvas));
        }

        // Through request-then-commit, as the tick path does it.
        void putInMode(AppMode wanted)
        {
            mode.request(wanted);
            mode.handle(
                antwika::event::TickEvent{
                    .tick = 0,
                    .event = antwika::event::Event{
                        .name = antwika::engine::events::kTick}});
        }

        [[nodiscard]] RenderSetup setup()
        {
            return RenderSetup{
                .window = window,
                .mode = mode,
                .canvas = kCanvas,
                .scene = scene,
                .atlas = atlas,
                .paths = paths,
                .camera = camera,
                .extent = kExtent,
                .overlay = overlay,
                .menuScene = menuScene,
                .menuOverlay = menuOverlay,
                .worldScene = worldScene,
                .cities = cities};
        }

        NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        Camera camera;
        const GridScene scene{};
        const MainMenuScene menuScene{};
        const WorldMapScene worldScene{};
        UiOverlay overlay;
        UiOverlay menuOverlay{kCanvas};
        NiceMock<MockTexture> atlas;
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;

        // The subject of most of these is the grid.
        // So a run is put on it rather than clicking its way there.
        AppModeState mode{AppMode::CityMap};

        // A small world, since only the mode branch is under test.
        WorldMapState cities{antwika::game::generateWorldMap(
            antwika::game::WorldMapConfig{
                .width = 6, .height = 6, .seed = 1})};
    };
} // namespace

TEST_F(RenderSystemTest, Update_DrawsAndThenPresentsExactlyOneFrame)
{
    RenderSystem system(setup());

    ::testing::InSequence order;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_ReadsTheWindowsSizeEveryTick)
{
    RenderSystem system(setup());

    // A resize needs no handling of its own, so long as it is re-read.
    EXPECT_CALL(window, size())
        .WillOnce(Return(kCanvas))
        .WillOnce(Return(Size{.width = 640, .height = 480}));

    system.update(world, 0);
    system.update(world, 1);
}

TEST_F(RenderSystemTest, Update_DrawsThePathsItIsGiven)
{
    paths.insert(Cell{.x = 0, .y = 0});

    RenderSystem system(setup());

    // The ground alone is one blit per cell.
    // A lone road adds its own, from the tile with no links.
    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height));
    EXPECT_CALL(renderer, drawTexture(Ref(atlas), roadTile(0), _, _));

    system.update(world, 0);
}

// A mode owns the whole screen.
// So in the menu no tile is blitted at all, whatever the grid holds.
TEST_F(RenderSystemTest, Update_DrawsTheMenuAndNoGridInTheMainMenuMode)
{
    paths.insert(Cell{.x = 0, .y = 0});
    putInMode(AppMode::MainMenu);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}

// Likewise for the world map: rectangles, and not one tile of any grid.
TEST_F(RenderSystemTest, Update_DrawsTheWorldMapAndNoGridInThatMode)
{
    paths.insert(Cell{.x = 0, .y = 0});
    putInMode(AppMode::WorldMap);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);
    EXPECT_CALL(renderer, clear(_));

    // Thirty-six tiles, and a marker for each of the four cities.
    EXPECT_CALL(renderer, drawRect(_, _)).Times(6 * 6 + 4);
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}
