#include <gtest/gtest.h>

#include <cmath>
#include <numbers>
#include <variant>

#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Math3D.hpp"

#include "MatrixApprox.hpp"

using antwika::gfx::Camera3D;
using antwika::gfx::getIdentityMatrix;
using antwika::gfx::Orthographic;
using antwika::gfx::Perspective;
using antwika::gfx::Vec3;
using antwika::gfx::Vec4;
using antwika::gfx::tests::getApproxEqual;
using antwika::gfx::tests::kEpsilon;

namespace
{
    constexpr float kQuarterTurn = std::numbers::pi_v<float> / 2.0F;

    Camera3D getLookingAtOriginFromZ()
    {
        return Camera3D{
            Vec3{0.0F, 0.0F, 5.0F},
            Vec3{0.0F, 0.0F, 0.0F},
            Vec3{0.0F, 1.0F, 0.0F},
            Perspective{}};
    }
}

TEST(PerspectiveTest, OperatorEquals_ComparesEveryField)
{
    const Perspective leftPerspective;
    Perspective rightPerspective;

    EXPECT_EQ(leftPerspective, rightPerspective);

    rightPerspective = leftPerspective;
    rightPerspective.fovYRadians = kQuarterTurn;
    EXPECT_NE(leftPerspective, rightPerspective);

    rightPerspective = leftPerspective;
    rightPerspective.aspectRatio = 2.0F;
    EXPECT_NE(leftPerspective, rightPerspective);

    rightPerspective = leftPerspective;
    rightPerspective.nearPlane = 0.5F;
    EXPECT_NE(leftPerspective, rightPerspective);

    rightPerspective = leftPerspective;
    rightPerspective.farPlane = 200.0F;
    EXPECT_NE(leftPerspective, rightPerspective);
}

TEST(OrthographicTest, OperatorEquals_ComparesEveryField)
{
    const Orthographic leftOrthographic;
    Orthographic rightOrthographic;

    EXPECT_EQ(leftOrthographic, rightOrthographic);

    rightOrthographic = leftOrthographic;
    rightOrthographic.halfWidth = 2.0F;
    EXPECT_NE(leftOrthographic, rightOrthographic);

    rightOrthographic = leftOrthographic;
    rightOrthographic.halfHeight = 2.0F;
    EXPECT_NE(leftOrthographic, rightOrthographic);

    rightOrthographic = leftOrthographic;
    rightOrthographic.nearPlane = -50.0F;
    EXPECT_NE(leftOrthographic, rightOrthographic);

    rightOrthographic = leftOrthographic;
    rightOrthographic.farPlane = 200.0F;
    EXPECT_NE(leftOrthographic, rightOrthographic);
}

TEST(OrthographicTest, OperatorEquals_ComparesWhereTheRectangleSits)
{
    const antwika::gfx::Orthographic leftOrthographic;
    antwika::gfx::Orthographic rightOrthographic;

    EXPECT_EQ(leftOrthographic, rightOrthographic);

    rightOrthographic.offsetX = 4.0F;
    EXPECT_NE(leftOrthographic, rightOrthographic);

    rightOrthographic = leftOrthographic;
    rightOrthographic.offsetY = -2.0F;
    EXPECT_NE(leftOrthographic, rightOrthographic);
}

TEST(Camera3DTest, View_DefaultCameraLooksDownNegativeZ)
{
    EXPECT_TRUE(getApproxEqual(Camera3D{}.getView(), getIdentityMatrix()));
}

TEST(Camera3DTest, View_PutsTheTargetInFrontOfTheEye)
{
    const Vec4 targetPosition =
        getLookingAtOriginFromZ().getView() * Vec4(0.0F, 0.0F, 0.0F, 1.0F);

    EXPECT_TRUE(
        getApproxEqual(targetPosition, Vec4(0.0F, 0.0F, -5.0F, 1.0F)));
}

TEST(Camera3DTest, View_EyeOnItsTargetIsTheIdentityRatherThanNaN)
{
    Camera3D camera = getLookingAtOriginFromZ();
    camera.setTarget(camera.getPosition());

    EXPECT_TRUE(getApproxEqual(camera.getView(), getIdentityMatrix()));
}

TEST(Camera3DTest, Accessors_ReportWhatTheCameraWasBuiltWith)
{
    const Camera3D camera = getLookingAtOriginFromZ();

    EXPECT_TRUE(getApproxEqual(camera.getPosition(), Vec3(0.0F, 0.0F, 5.0F)));
    EXPECT_TRUE(getApproxEqual(camera.getTarget(), Vec3(0.0F, 0.0F, 0.0F)));
    EXPECT_TRUE(getApproxEqual(camera.getUp(), Vec3(0.0F, 1.0F, 0.0F)));
    EXPECT_EQ(Perspective{}, std::get<Perspective>(camera.getProjection()));
}

TEST(Camera3DTest, SetPosition_MovesTheEye)
{
    Camera3D camera = getLookingAtOriginFromZ();
    camera.setPosition(Vec3{0.0F, 0.0F, 9.0F});

    const Vec4 targetPoint =
        camera.getView() * Vec4(0.0F, 0.0F, 0.0F, 1.0F);

    EXPECT_TRUE(getApproxEqual(camera.getPosition(), Vec3(0.0F, 0.0F, 9.0F)));
    EXPECT_TRUE(getApproxEqual(targetPoint, Vec4(0.0F, 0.0F, -9.0F, 1.0F)));
}

TEST(Camera3DTest, SetTarget_AimsTheEye)
{
    Camera3D camera = getLookingAtOriginFromZ();
    camera.setTarget(Vec3{1.0F, 0.0F, 5.0F});

    EXPECT_TRUE(getApproxEqual(camera.getTarget(), Vec3(1.0F, 0.0F, 5.0F)));
}

TEST(Camera3DTest, SetProjection_ReplacesTheOneHeld)
{
    Camera3D camera = getLookingAtOriginFromZ();
    const Orthographic wantedOrthographic{
        .halfWidth = 2.0F,
        .halfHeight = 1.0F,
        .nearPlane = -1.0F,
        .farPlane = 1.0F};

    camera.setProjection(wantedOrthographic);

    EXPECT_EQ(wantedOrthographic, std::get<Orthographic>(camera.getProjection()));
}

TEST(Camera3DTest, ProjectionMatrix_PerspectiveScalesByFieldOfView)
{
    Camera3D camera;
    camera.setProjection(Perspective{
        .fovYRadians = kQuarterTurn,
        .aspectRatio = 2.0F,
        .nearPlane = 1.0F,
        .farPlane = 3.0F});

    const auto matrix = camera.getProjectionMatrix();
    const float focal = 1.0F / std::tan(kQuarterTurn / 2.0F);

    EXPECT_NEAR(focal / 2.0F, matrix[0][0], kEpsilon);
    EXPECT_NEAR(focal, matrix[1][1], kEpsilon);
    EXPECT_NEAR(-1.0F, matrix[2][3], kEpsilon);
}

TEST(Camera3DTest, ProjectionMatrix_PerspectiveKeepsNearAndFarInRange)
{
    Camera3D camera;
    camera.setProjection(Perspective{
        .fovYRadians = kQuarterTurn,
        .aspectRatio = 1.0F,
        .nearPlane = 1.0F,
        .farPlane = 3.0F});

    const auto matrix = camera.getProjectionMatrix();
    const Vec4 onNearPoint = matrix * Vec4(0.0F, 0.0F, -1.0F, 1.0F);
    const Vec4 onFarPoint = matrix * Vec4(0.0F, 0.0F, -3.0F, 1.0F);

    EXPECT_NEAR(-1.0F, onNearPoint.z / onNearPoint.w, kEpsilon);
    EXPECT_NEAR(1.0F, onFarPoint.z / onFarPoint.w, kEpsilon);
}

TEST(Camera3DTest, ProjectionMatrix_OrthographicMapsHalfExtentsToOne)
{
    Camera3D camera;
    camera.setProjection(Orthographic{
        .halfWidth = 2.0F,
        .halfHeight = 1.0F,
        .nearPlane = -1.0F,
        .farPlane = 1.0F});

    const Vec4 corner =
        camera.getProjectionMatrix() * Vec4(2.0F, 1.0F, 0.0F, 1.0F);

    EXPECT_TRUE(getApproxEqual(corner, Vec4(1.0F, 1.0F, 0.0F, 1.0F)));
}

TEST(Camera3DTest, ViewProjection_IsTheProjectionTimesTheView)
{
    const Camera3D camera = getLookingAtOriginFromZ();

    ASSERT_FALSE(getApproxEqual(camera.getView(), getIdentityMatrix()));
    ASSERT_FALSE(getApproxEqual(camera.getProjectionMatrix(), getIdentityMatrix()));
    ASSERT_FALSE(getApproxEqual(
        camera.getProjectionMatrix() * camera.getView(),
        camera.getView() * camera.getProjectionMatrix()));

    EXPECT_TRUE(getApproxEqual(
        camera.getViewProjection(),
        camera.getProjectionMatrix() * camera.getView()));
}
