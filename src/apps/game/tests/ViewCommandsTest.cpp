#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/ViewCommands.hpp"

using antwika::game::Camera;
using antwika::game::PauseState;
using antwika::game::ViewCommands;

namespace
{
    constexpr antwika::gfx::Point kHome{.x = 100, .y = 50};

    class ViewCommandsTest : public ::testing::Test
    {
    protected:
        Camera camera{kHome};
        PauseState pause;
        ViewCommands view{camera, pause, Camera{kHome}};
    };
} // namespace

TEST_F(ViewCommandsTest, ZoomIn_TakesTheViewOneStepCloser)
{
    const auto before = camera.zoomLevel();

    view.zoomIn();

    EXPECT_EQ(camera.zoomLevel(), before + 1);
}

TEST_F(ViewCommandsTest, ZoomOut_TakesTheViewOneStepOut)
{
    const auto before = camera.zoomLevel();

    view.zoomOut();

    EXPECT_EQ(camera.zoomLevel(), before - 1);
}

// The camera the run was configured with, not wherever it drifted to.
TEST_F(ViewCommandsTest, ResetView_PutsTheCameraBackWhereItStarted)
{
    camera.panBy(40, 8);
    view.zoomIn();

    view.resetView();

    EXPECT_EQ(camera, Camera{kHome});
}

// The opposite of what is showing rather than a flip of a flag.
TEST_F(ViewCommandsTest, TogglePause_HoldsTheRunAndLetsItGo)
{
    ASSERT_FALSE(pause.paused());

    view.togglePause();
    EXPECT_TRUE(pause.paused());

    view.togglePause();
    EXPECT_FALSE(pause.paused());
}
