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
}

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
    const SpinScene first;
    const SpinScene second;

    EXPECT_EQ(first.modelAt(37), second.modelAt(37));

    EXPECT_EQ(first.modelAt(900), second.modelAt(900));
    EXPECT_EQ(second.modelAt(0), first.modelAt(0));

    const auto turned = first.modelAt(37);

    EXPECT_NEAR(turned[0][0], 0.720346F, 1e-5F);
    EXPECT_NEAR(turned[0][2], -0.674288F, 1e-5F);
    EXPECT_NEAR(turned[1][1], 0.921001F, 1e-5F);
    EXPECT_NEAR(turned[2][0], 0.680473F, 1e-5F);
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

    EXPECT_CALL(renderer, present()).Times(0);

    scene.draw(renderer, renderer, mesh, kCanvas, 0);
}
