#include "antwika/life/RenderSystem.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/BoardScene.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/Grid.hpp"

using antwika::ecs::World;
using antwika::gfx::Color;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::life::BoardScene;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::life::RenderSystem;
using antwika::log::mocks::MockLogger;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr Color kDeadCells{.red = 32, .green = 36, .blue = 48};
    constexpr Color kAliveCell{.red = 96, .green = 224, .blue = 128};

    // A 2x2 world with only the top-right cell alive.
    void seedOneLiveCell(World &world, const Grid &grid)
    {
        world.commit();
        world.set<Cell>(grid.entityAt(1, 0), Cell{.alive = true});
        world.commit();
    }
} // namespace

TEST(RenderSystemTest, Update_DrawsTheWorldsCellsAndPresents)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    seedOneLiveCell(world, grid);

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, configuredSize())
        .WillByDefault(Return(Size{.width = 20, .height = 20}));

    const InSequence sequence;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 20, .height = 20}},
            kDeadCells));
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 10, .y = 0},
                .size = {.width = 10, .height = 10}},
            kAliveCell));
    EXPECT_CALL(renderer, present());

    const BoardScene scene;
    RenderSystem system(window, scene, 2, 2);
    system.update(world, 0);
}

// The size is asked for again every tick rather than kept.
// So this system holds no copy of a number the window owns.
TEST(RenderSystemTest, Update_ReReadsTheConfiguredSizeEveryTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    seedOneLiveCell(world, grid);

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    EXPECT_CALL(window, configuredSize())
        .Times(2)
        .WillRepeatedly(Return(Size{.width = 20, .height = 20}));

    const BoardScene scene;
    RenderSystem system(window, scene, 2, 2);
    system.update(world, 0);
    system.update(world, 1);
}

// Where a cell is drawn and which cell a click lands in are one.
// One function, and only while the two take one size.
// PointerToggleSink lays out from the configured size.
// A window manager handing back a size of its own must move nothing.
// See docs/resizable-windows.md.
TEST(RenderSystemTest, Update_DrawsAgainstTheConfiguredSizeNotTheReported)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    seedOneLiveCell(world, grid);

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, configuredSize())
        .WillByDefault(Return(Size{.width = 20, .height = 20}));
    ON_CALL(window, size())
        .WillByDefault(Return(Size{.width = 40, .height = 40}));

    const InSequence sequence;
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 20, .height = 20}},
            kDeadCells));
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 10, .y = 0},
                .size = {.width = 10, .height = 10}},
            kAliveCell));

    const BoardScene scene;
    RenderSystem system(window, scene, 2, 2);
    system.update(world, 0);
}

// Rendering is a projection of state, so it must not become an input to it.
TEST(RenderSystemTest, Update_LeavesTheWorldUnchanged)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    seedOneLiveCell(world, grid);

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, configuredSize())
        .WillByDefault(Return(Size{.width = 20, .height = 20}));

    const auto before = antwika::life::readBoard(world, grid);

    const BoardScene scene;
    RenderSystem system(window, scene, 2, 2);
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(antwika::life::readBoard(world, grid), before);
}
