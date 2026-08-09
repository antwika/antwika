#include <gtest/gtest.h>

#include <cmath>
#include <numbers>
#include <variant>

#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Math3D.hpp"

#include "MatrixApprox.hpp"

using antwika::gfx::Camera3D;
using antwika::gfx::identityMatrix;
using antwika::gfx::Orthographic;
using antwika::gfx::Perspective;
using antwika::gfx::Vec3;
using antwika::gfx::Vec4;
using antwika::gfx::tests::approxEqual;
using antwika::gfx::tests::kEpsilon;

namespace
{
    constexpr float kQuarterTurn = std::numbers::pi_v<float> / 2.0F;

    Camera3D lookingAtOriginFromZ()
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
    const Perspective left;
    Perspective right;

    EXPECT_EQ(left, right);

    right = left;
    right.fovYRadians = kQuarterTurn;
    EXPECT_NE(left, right);

    right = left;
    right.aspectRatio = 2.0F;
    EXPECT_NE(left, right);

    right = left;
    right.nearPlane = 0.5F;
    EXPECT_NE(left, right);

    right = left;
    right.farPlane = 200.0F;
    EXPECT_NE(left, right);
}

TEST(OrthographicTest, OperatorEquals_ComparesEveryField)
{
    const Orthographic left;
    Orthographic right;

    EXPECT_EQ(left, right);

    right = left;
    right.halfWidth = 2.0F;
    EXPECT_NE(left, right);

    right = left;
    right.halfHeight = 2.0F;
    EXPECT_NE(left, right);

    right = left;
    right.nearPlane = -50.0F;
    EXPECT_NE(left, right);

    right = left;
    right.farPlane = 200.0F;
    EXPECT_NE(left, right);
}

TEST(Camera3DTest, View_DefaultCameraLooksDownNegativeZ)
{
    EXPECT_TRUE(approxEqual(Camera3D{}.view(), identityMatrix()));
}

TEST(Camera3DTest, View_PutsTheTargetInFrontOfTheEye)
{
    const Vec4 target =
        lookingAtOriginFromZ().view() * Vec4(0.0F, 0.0F, 0.0F, 1.0F);

    EXPECT_TRUE(approxEqual(target, Vec4(0.0F, 0.0F, -5.0F, 1.0F)));
}

TEST(Camera3DTest, View_EyeOnItsTargetIsTheIdentityRatherThanNaN)
{
    Camera3D camera = lookingAtOriginFromZ();
    camera.setTarget(camera.position());

    EXPECT_TRUE(approxEqual(camera.view(), identityMatrix()));
}

TEST(Camera3DTest, Accessors_ReportWhatTheCameraWasBuiltWith)
{
    const Camera3D camera = lookingAtOriginFromZ();

    EXPECT_TRUE(approxEqual(camera.position(), Vec3(0.0F, 0.0F, 5.0F)));
    EXPECT_TRUE(approxEqual(camera.target(), Vec3(0.0F, 0.0F, 0.0F)));
    EXPECT_TRUE(approxEqual(camera.up(), Vec3(0.0F, 1.0F, 0.0F)));
    EXPECT_EQ(Perspective{}, std::get<Perspective>(camera.projection()));
}

TEST(Camera3DTest, SetPosition_MovesTheEye)
{
    Camera3D camera = lookingAtOriginFromZ();
    camera.setPosition(Vec3{0.0F, 0.0F, 9.0F});

    const Vec4 target =
        camera.view() * Vec4(0.0F, 0.0F, 0.0F, 1.0F);

    EXPECT_TRUE(approxEqual(camera.position(), Vec3(0.0F, 0.0F, 9.0F)));
    EXPECT_TRUE(approxEqual(target, Vec4(0.0F, 0.0F, -9.0F, 1.0F)));
}

TEST(Camera3DTest, SetTarget_AimsTheEye)
{
    Camera3D camera = lookingAtOriginFromZ();
    camera.setTarget(Vec3{1.0F, 0.0F, 5.0F});

    EXPECT_TRUE(approxEqual(camera.target(), Vec3(1.0F, 0.0F, 5.0F)));
}

TEST(Camera3DTest, SetProjection_ReplacesTheOneHeld)
{
    Camera3D camera = lookingAtOriginFromZ();
    const Orthographic wanted{
        .halfWidth = 2.0F,
        .halfHeight = 1.0F,
        .nearPlane = -1.0F,
        .farPlane = 1.0F};

    camera.setProjection(wanted);

    EXPECT_EQ(wanted, std::get<Orthographic>(camera.projection()));
}

TEST(Camera3DTest, ProjectionMatrix_PerspectiveScalesByFieldOfView)
{
    Camera3D camera;
    camera.setProjection(Perspective{
        .fovYRadians = kQuarterTurn,
        .aspectRatio = 2.0F,
        .nearPlane = 1.0F,
        .farPlane = 3.0F});

    const auto matrix = camera.projectionMatrix();
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

    const auto matrix = camera.projectionMatrix();
    const Vec4 onNear = matrix * Vec4(0.0F, 0.0F, -1.0F, 1.0F);
    const Vec4 onFar = matrix * Vec4(0.0F, 0.0F, -3.0F, 1.0F);

    EXPECT_NEAR(-1.0F, onNear.z / onNear.w, kEpsilon);
    EXPECT_NEAR(1.0F, onFar.z / onFar.w, kEpsilon);
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
        camera.projectionMatrix() * Vec4(2.0F, 1.0F, 0.0F, 1.0F);

    EXPECT_TRUE(approxEqual(corner, Vec4(1.0F, 1.0F, 0.0F, 1.0F)));
}

TEST(Camera3DTest, ViewProjection_IsTheProjectionTimesTheView)
{
    const Camera3D camera = lookingAtOriginFromZ();

    ASSERT_FALSE(approxEqual(camera.view(), identityMatrix()));
    ASSERT_FALSE(approxEqual(camera.projectionMatrix(), identityMatrix()));
    ASSERT_FALSE(approxEqual(
        camera.projectionMatrix() * camera.view(),
        camera.view() * camera.projectionMatrix()));

    EXPECT_TRUE(approxEqual(
        camera.viewProjection(),
        camera.projectionMatrix() * camera.view()));
}
