#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <variant>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockRenderer3D.hpp>

#include "antwika/gfx3d_demo/SpinScene.hpp"

using antwika::gfx::Camera3D;
using antwika::gfx::Color;
using antwika::gfx::identityMatrix;
using antwika::gfx::Perspective;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::mocks::MockRenderer3D;
using antwika::gfx3d_demo::SpinScene;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 800, .height = 600};
} // namespace

TEST(SpinSceneTest, ModelAt_IsTheIdentityBeforeAnyTickHasPassed)
{
    const SpinScene scene;

    EXPECT_EQ(scene.modelAt(0), identityMatrix());
}

TEST(SpinSceneTest, ModelAt_TurnsFurtherAsTicksPass)
{
    const SpinScene scene;

    EXPECT_NE(scene.modelAt(1), scene.modelAt(0));
    EXPECT_NE(scene.modelAt(2), scene.modelAt(1));
}

TEST(SpinSceneTest, ModelAt_DependsOnNothingButTheTick)
{
    // The point of the whole app: no clock is read anywhere.
    // Two scenes built apart agree about a tick neither has reached.
    // Asking one of them twice about it agrees too.
    const SpinScene first;
    const SpinScene second;

    EXPECT_EQ(first.modelAt(37), second.modelAt(37));
    EXPECT_EQ(first.modelAt(37), first.modelAt(37));

    // Out of order, so nothing can be answering from what came before.
    EXPECT_EQ(first.modelAt(900), second.modelAt(900));
    EXPECT_EQ(second.modelAt(0), first.modelAt(0));
}

TEST(SpinSceneTest, CameraFor_TakesItsProportionsFromTheCanvas)
{
    const SpinScene scene;

    const auto camera = scene.cameraFor(kCanvas);
    const auto projection = std::get<Perspective>(camera.projection());

    EXPECT_FLOAT_EQ(projection.aspectRatio, 800.0F / 600.0F);
}

TEST(SpinSceneTest, CameraFor_LooksAtTheOrigin)
{
    const SpinScene scene;

    const auto camera = scene.cameraFor(kCanvas);

    EXPECT_EQ(camera.target(), antwika::gfx::Vec3(0.0F, 0.0F, 0.0F));
    EXPECT_NE(camera.position(), camera.target());
}

TEST(SpinSceneTest, CameraFor_FallsBackWhenTheCanvasHasNoHeight)
{
    const SpinScene scene;

    // Dividing by it would put a NaN into every matrix that follows.
    const auto camera =
        scene.cameraFor(Size{.width = 800, .height = 0});
    const auto projection = std::get<Perspective>(camera.projection());

    EXPECT_FLOAT_EQ(projection.aspectRatio, 1.0F);
}

TEST(SpinSceneTest, Draw_ClearsThenDrawsTheCubeThenTheCaption)
{
    const SpinScene scene;
    NiceMock<MockRenderer3D> renderer;
    const NiceMock<MockMesh> mesh;

    // Order is the whole assertion here.
    // There is one frame and both halves draw into it.
    // What goes last goes in front.
    const InSequence order;

    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(
        renderer,
        drawMesh(::testing::Ref(mesh), scene.modelAt(4), _, _));
    EXPECT_CALL(renderer, drawText(_, _, _, _));

    scene.draw(renderer, renderer, mesh, kCanvas, 4);
}

TEST(SpinSceneTest, Draw_NeverPresentsTheFrameItself)
{
    const SpinScene scene;
    NiceMock<MockRenderer3D> renderer;
    const NiceMock<MockMesh> mesh;

    // Presenting belongs to whoever owns the frame, not to a scene:
    // an app drawing two scenes would otherwise show the first alone.
    EXPECT_CALL(renderer, present()).Times(0);

    scene.draw(renderer, renderer, mesh, kCanvas, 0);
}
