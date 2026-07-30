#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::ecs::World;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::PathIndex;
using antwika::game::RenderSystem;
using antwika::game::UiOverlay;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::log::mocks::MockLogger;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
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

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, size()).WillByDefault(Return(kCanvas));

    RenderSystem system(window, scene, paths, camera, kExtent, overlay);

    ::testing::InSequence order;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());
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

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

    // A resize needs no handling of its own, so long as it is re-read.
    EXPECT_CALL(window, size())
        .WillOnce(Return(kCanvas))
        .WillOnce(Return(Size{.width = 640, .height = 480}));

    RenderSystem system(window, scene, paths, camera, kExtent, overlay);

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

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, size()).WillByDefault(Return(kCanvas));

    RenderSystem system(window, scene, paths, camera, kExtent, overlay);

    // The lattice alone is two lines per cell.
    // A filled tile adds a row per pixel of its height on top.
    std::size_t lines = 0;
    ON_CALL(renderer, drawLine(_, _, _))
        .WillByDefault([&lines](auto, auto, auto) { ++lines; });

    system.update(world, 0);

    EXPECT_GT(
        lines,
        static_cast<std::size_t>(2 * kExtent.width * kExtent.height));
}
