#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/TileAtlas.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::ecs::World;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::MainMenuScene;
using antwika::game::PathIndex;
using antwika::game::RenderSystem;
using antwika::game::roadTile;
using antwika::game::UiOverlay;
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
} // namespace

TEST(RenderSystemTest, Update_DrawsAndThenPresentsExactlyOneFrame)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    PathIndex paths;
    const Camera camera;
    const GridScene scene;
    const UiOverlay overlay;
    NiceMock<MockTexture> atlas;

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, size()).WillByDefault(Return(kCanvas));

    AppModeState mode{AppMode::Playing};
    const MainMenuScene menuScene;
    UiOverlay menuOverlay;
    RenderSystem system(
        window,
        scene,
        atlas,
        paths,
        camera,
        kExtent,
        overlay,
        mode,
        menuScene,
        menuOverlay);

    ::testing::InSequence order;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}

TEST(RenderSystemTest, Update_ReadsTheWindowsSizeEveryTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    PathIndex paths;
    const Camera camera;
    const GridScene scene;
    const UiOverlay overlay;
    NiceMock<MockTexture> atlas;

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

    // A resize needs no handling of its own, so long as it is re-read.
    EXPECT_CALL(window, size())
        .WillOnce(Return(kCanvas))
        .WillOnce(Return(Size{.width = 640, .height = 480}));

    AppModeState mode{AppMode::Playing};
    const MainMenuScene menuScene;
    UiOverlay menuOverlay;
    RenderSystem system(
        window,
        scene,
        atlas,
        paths,
        camera,
        kExtent,
        overlay,
        mode,
        menuScene,
        menuOverlay);

    system.update(world, 0);
    system.update(world, 1);
}

TEST(RenderSystemTest, Update_DrawsThePathsItIsGiven)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    PathIndex paths;
    paths.insert(Cell{.x = 0, .y = 0});
    const Camera camera;
    const GridScene scene;
    const UiOverlay overlay;
    NiceMock<MockTexture> atlas;

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, size()).WillByDefault(Return(kCanvas));

    AppModeState mode{AppMode::Playing};
    const MainMenuScene menuScene;
    UiOverlay menuOverlay;
    RenderSystem system(
        window,
        scene,
        atlas,
        paths,
        camera,
        kExtent,
        overlay,
        mode,
        menuScene,
        menuOverlay);

    // The ground alone is one blit per cell.
    // A lone road adds its own, from the tile with no links.
    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height));
    EXPECT_CALL(renderer, drawTexture(Ref(atlas), roadTile(0), _, _));

    system.update(world, 0);
}

// A mode owns the whole screen.
// So in the menu no tile is blitted at all, whatever the grid holds.
TEST(RenderSystemTest, Update_DrawsTheMenuAndNoGridInTheMainMenuMode)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    PathIndex paths;
    paths.insert(Cell{.x = 0, .y = 0});
    Camera camera;
    const GridScene scene;
    UiOverlay overlay;
    NiceMock<MockWindow> window;
    NiceMock<MockRenderer> renderer;
    NiceMock<MockTexture> atlas;

    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, size()).WillByDefault(Return(kCanvas));

    AppModeState mode;
    const MainMenuScene menuScene;
    UiOverlay menuOverlay{kCanvas};
    RenderSystem system(
        window,
        scene,
        atlas,
        paths,
        camera,
        kExtent,
        overlay,
        mode,
        menuScene,
        menuOverlay);

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}
