#include <gtest/gtest.h>

#include <glm/geometric.hpp>

#include <cmath>
#include <variant>
#include <numbers>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/Size.hpp>

#include <antwika/camera/FlyCamera.hpp>

using antwika::camera::CameraTransform;
using antwika::camera::cameraOf;
using antwika::camera::getForwardOnGround;
using antwika::camera::kMaxPitch;
using antwika::camera::getForward;
using antwika::camera::getPanned;
using antwika::camera::kPanStep;
using antwika::camera::getCenteredOn;
using antwika::camera::getDefaultTransform;
using antwika::gfx::Size;
using antwika::camera::getResetToIsometric;
using antwika::camera::getRotated;
using antwika::camera::getMovedOnGround;
using antwika::gfx::Vec3;

namespace
{
    constexpr float kTolerance = 0.001F;

    constexpr auto kQuarterTurn = std::numbers::pi_v<float> / 2.0F;

    constexpr antwika::gfx::Size kCanvasSize{
        .width = 320, .height = 180};
}

TEST(FlyCameraTest, Forward_TakesTheMapsOwnWayInWithNoTurn)
{
    const auto way = getForward(CameraTransform{});

    EXPECT_NEAR(way.x, 0.0F, kTolerance);
    EXPECT_NEAR(way.y, 0.0F, kTolerance);
    EXPECT_NEAR(way.z, -1.0F, kTolerance);
}

TEST(FlyCameraTest, Forward_TurnsToTheRightAsTheYawGrows)
{
    const auto way = getForward(CameraTransform{.yaw = kQuarterTurn});

    EXPECT_NEAR(way.x, 1.0F, kTolerance);
    EXPECT_NEAR(way.z, 0.0F, kTolerance);
}

TEST(FlyCameraTest, Forward_DipsBelowTheHorizonOnALesserPitch)
{
    const auto way = getForward(CameraTransform{.pitch = -0.5F});

    EXPECT_LT(way.y, 0.0F);
    EXPECT_NEAR(glm::length(way), 1.0F, kTolerance);
}

TEST(FlyCameraTest, ForwardOnGround_StaysLevelHoweverFarTheCameraTips)
{
    for (const float pitch : {-1.4F, -0.5F, 0.0F, 1.4F})
    {
        const auto way = getForwardOnGround(
            CameraTransform{.yaw = 0.8F,
            .pitch = pitch});

        EXPECT_NEAR(way.y, 0.0F, kTolerance) << pitch;
        EXPECT_NEAR(glm::length(way), 1.0F, kTolerance) << pitch;
    }
}

TEST(FlyCameraTest, Rotated_LeavesTheCameraStandingWhereItWas)
{
    const CameraTransform transform{.position = Vec3{3.0F, 4.0F, 5.0F}};
    const auto rotatedTransform = getRotated(transform, 1.2F, -0.4F);

    EXPECT_EQ(rotatedTransform.position, transform.position);
    EXPECT_NEAR(rotatedTransform.yaw, 1.2F, kTolerance);
    EXPECT_NEAR(rotatedTransform.pitch, -0.4F, kTolerance);
}

TEST(FlyCameraTest, Rotated_StopsShortOfStraightUpAndStraightDown)
{
    EXPECT_NEAR(
        getRotated(CameraTransform{}, 0.0F, 100.0F).pitch,
        kMaxPitch,
        kTolerance);
    EXPECT_NEAR(
        getRotated(CameraTransform{}, 0.0F, -100.0F).pitch,
        -kMaxPitch,
        kTolerance);
}

TEST(FlyCameraTest, MovedOnGround_GoesTheWayItFacesAndToItsRight)
{
    const auto ahead = getMovedOnGround(CameraTransform{}, 1.0F, 0.0F, 2.0F);

    EXPECT_NEAR(ahead.position.x, 0.0F, kTolerance);
    EXPECT_NEAR(ahead.position.z, -2.0F, kTolerance);

    const auto movedTransform = getMovedOnGround(CameraTransform{}, 0.0F, 1.0F,
        2.0F);

    EXPECT_NEAR(movedTransform.position.x, 2.0F, kTolerance);
    EXPECT_NEAR(movedTransform.position.z, 0.0F, kTolerance);
}

TEST(FlyCameraTest, MovedOnGround_KeepsToItsOwnHeightHoweverItLooks)
{
    const CameraTransform transform{
        .position = Vec3{0.0F, 5.0F, 0.0F}, .pitch = -1.2F};

    EXPECT_NEAR(
        getMovedOnGround(transform, 1.0F, 0.0F, 3.0F).position.y,
        5.0F,
        kTolerance);
}

TEST(FlyCameraTest, MovedOnGround_CoversTheSameGroundOnADiagonal)
{
    const auto straight = getMovedOnGround(CameraTransform{}, 1.0F, 0.0F, 1.0F);
    const auto corner = getMovedOnGround(CameraTransform{}, 1.0F, 1.0F, 1.0F);

    EXPECT_NEAR(
        glm::length(corner.position),
        glm::length(straight.position),
        kTolerance);
}

TEST(FlyCameraTest, MovedOnGround_StandsStillWhenAskedForNothing)
{
    const CameraTransform transform{.position = Vec3{1.0F, 2.0F, 3.0F}};

    EXPECT_EQ(getMovedOnGround(transform, 0.0F, 0.0F, 1.0F), transform);
}

TEST(FlyCameraTest, CameraOf_StandsWhereTheTransformDoesAndLooksItsWay)
{
    const auto transform = getDefaultTransform();
    const auto camera = cameraOf(transform, kCanvasSize, 2.4F);

    EXPECT_NEAR(camera.getPosition().x, transform.position.x, kTolerance);
    EXPECT_NEAR(camera.getPosition().y, transform.position.y, kTolerance);
    EXPECT_NEAR(camera.getPosition().z, transform.position.z, kTolerance);

    const auto way =
        glm::normalize(camera.getTarget() - camera.getPosition());
    const auto forwardDirection = getForward(transform);

    EXPECT_NEAR(way.x, forwardDirection.x, kTolerance);
    EXPECT_NEAR(way.y, forwardDirection.y, kTolerance);
    EXPECT_NEAR(way.z, forwardDirection.z, kTolerance);
}

TEST(FlyCameraTest, DefaultTransform_StandsBackAndLooksDownAtTheOrigin)
{
    const auto transform = getDefaultTransform();
    const auto way = getForward(transform);

    EXPECT_GT(transform.position.y, 0.0F);
    EXPECT_GT(transform.position.z, 0.0F);
    EXPECT_LT(way.y, 0.0F);

    const auto ontoPoint =
        transform.position + (way * glm::length(transform.position));

    EXPECT_NEAR(glm::length(ontoPoint), 0.0F, 0.01F);
}

TEST(FlyCameraTest, Panned_MovesTheCameraWithoutTurningIt)
{
    const auto was = getDefaultTransform();
    const auto pannedTransform = getPanned(was, 3.0F, 2.0F, kPanStep);

    EXPECT_FLOAT_EQ(pannedTransform.yaw, was.yaw);
    EXPECT_FLOAT_EQ(pannedTransform.pitch, was.pitch);
    EXPECT_NE(pannedTransform.position, was.position);
}

TEST(FlyCameraTest, Panned_ComesBackWhereItBeganGoingBothWays)
{
    const auto was = getDefaultTransform();
    const auto pannedTransform = getPanned(was, 4.0F, -2.0F, kPanStep);
    const auto pannedBack = getPanned(pannedTransform, -4.0F, 2.0F, kPanStep);

    EXPECT_NEAR(glm::length(pannedBack.position - was.position), 0.0F, 1e-5F);
}

TEST(FlyCameraTest, Panned_SlidesAcrossTheViewAndNotAlongIt)
{
    const auto was = getDefaultTransform();
    const auto pannedTransform = getPanned(was, 5.0F, 0.0F, kPanStep);
    const auto forwardDirection = getForward(was);

    EXPECT_NEAR(
        glm::dot(pannedTransform.position - was.position, forwardDirection),
        0.0F,
        1e-5F);
}

TEST(FlyCameraTest, Panned_StaysWhereItIsAskedForNothing)
{
    const auto was = getDefaultTransform();

    EXPECT_EQ(getPanned(was, 0.0F, 0.0F, kPanStep), was);
}

TEST(FlyCameraTest, Squared_LooksForwardAndPitchedDownAgain)
{
    const auto rotatedTransform = getRotated(getDefaultTransform(), 1.2F, 0.4F);
    const auto squaredTransform = getResetToIsometric(rotatedTransform);

    EXPECT_FLOAT_EQ(squaredTransform.yaw, 0.0F);
    EXPECT_FLOAT_EQ(squaredTransform.pitch, getDefaultTransform().pitch);
}

TEST(FlyCameraTest, Squared_LeavesTheCameraWhereItStands)
{
    const auto movedTransform = getPanned(
        getRotated(getDefaultTransform(), 1.0F, 0.2F), 9.0F, 4.0F, kPanStep);

    EXPECT_EQ(getResetToIsometric(movedTransform).position,
        movedTransform.position);
}

TEST(FlyCameraTest, Squared_ChangesNothingAboutTheOpeningTransform)
{
    EXPECT_EQ(getResetToIsometric(getDefaultTransform()), getDefaultTransform());
}

TEST(FlyCameraTest, CameraOf_KeepsDrawingWhatItIsFlownPast)
{
    const auto camera = cameraOf(
        getDefaultTransform(), Size{.width = 320, .height = 180}, 2.0F);
    const auto *ortho =
        std::get_if<antwika::gfx::Orthographic>(&camera.getProjection());

    ASSERT_NE(ortho, nullptr);
    EXPECT_LT(ortho->nearPlane, 0.0F);
    EXPECT_GT(ortho->farPlane, 0.0F);
}

TEST(FlyCameraTest, CameraOf_KeepsAsMuchBehindItAsBefore)
{
    const auto camera = cameraOf(
        getDefaultTransform(), Size{.width = 320, .height = 180}, 2.0F);
    const auto *ortho =
        std::get_if<antwika::gfx::Orthographic>(&camera.getProjection());

    ASSERT_NE(ortho, nullptr);
    EXPECT_FLOAT_EQ(ortho->nearPlane, -ortho->farPlane);
}

TEST(FlyCameraTest, CenteredOn_MovesTheCameraToFrameAPlace)
{
    const antwika::gfx::Vec3 awayPosition{9.0F, 0.0F, 4.0F};
    const auto transform = getCenteredOn(getDefaultTransform(), awayPosition);

    EXPECT_EQ(transform.position, getDefaultTransform().position + awayPosition);
    EXPECT_FLOAT_EQ(transform.yaw, getDefaultTransform().yaw);
    EXPECT_FLOAT_EQ(transform.pitch, getDefaultTransform().pitch);
}

TEST(FlyCameraTest, CenteredOn_LeavesTheDefaultTransformAloneForTheOrigin)
{
    EXPECT_EQ(
        getCenteredOn(getDefaultTransform(), antwika::gfx::Vec3{0.0F, 0.0F, 0.0F}),
        getDefaultTransform());
}

TEST(FlyCameraTest, SquaredPitch_IsTheAngleWhoseTangentIsFourThirds)
{
    using antwika::camera::getIsometricPitch;

    EXPECT_NEAR(std::tan(-getIsometricPitch()), 4.0F / 3.0F, 1e-6F);
    EXPECT_NEAR(std::sin(-getIsometricPitch()), 0.8F, 1e-6F);
    EXPECT_NEAR(std::cos(-getIsometricPitch()), 0.6F, 1e-6F);
}

TEST(FlyCameraTest, SnappedPitch_MakesAPitchWrittenBackSquareAgain)
{
    using antwika::camera::getIsometricPitch;
    using antwika::camera::getSnappedPitch;

    const auto roundedPitch =
        static_cast<float>(std::llround(getIsometricPitch() * 1000.0F))
        / 1000.0F;

    EXPECT_NE(roundedPitch, getIsometricPitch());
    EXPECT_EQ(
        getSnappedPitch(CameraTransform{.pitch = roundedPitch}).pitch,
        getIsometricPitch());
}

TEST(FlyCameraTest, SnappedPitch_LeavesAPitchFlownElsewhereAsItWas)
{
    using antwika::camera::getSnappedPitch;

    EXPECT_EQ(getSnappedPitch(CameraTransform{.pitch = -0.5F}).pitch, -0.5F);
    EXPECT_EQ(getSnappedPitch(CameraTransform{.pitch = 0.0F}).pitch, 0.0F);
}

TEST(FlyCameraTest, OrthoHalfHeight_DrawsAVoxelTheAskedPixelsAcross)
{
    using antwika::camera::kCanvasSize;
    using antwika::camera::kVoxelPixels;
    using antwika::camera::getOrthoHalfHeight;

    for (const std::int32_t pixelsPerVoxel :
         {kVoxelPixels, kVoxelPixels * 2, kVoxelPixels * 3})
    {
        const auto halfHeight =
            getOrthoHalfHeight(kCanvasSize, pixelsPerVoxel);
        const auto pixels = static_cast<float>(kCanvasSize.height)
                            / (2.0F * halfHeight);

        EXPECT_NEAR(pixels, static_cast<float>(pixelsPerVoxel), 1e-3F);
    }
}

TEST(FlyCameraTest, CameraOf_StandsWhereTheTransformStands)
{
    using antwika::camera::kCanvasSize;
    using antwika::camera::kVoxelPixels;
    using antwika::camera::getIsometricPitch;
    using antwika::camera::getOrthoHalfHeight;

    const auto halfHeight =
        getOrthoHalfHeight(kCanvasSize, kVoxelPixels);
    const auto pixel =
        2.0F * halfHeight / static_cast<float>(kCanvasSize.height);
    const auto square = [halfHeight, pixel](const float acrossPixels)
    {
        return cameraOf(
                   CameraTransform{
                       .position =
                           antwika::gfx::Vec3{
                               acrossPixels * pixel, 0.0F, 0.0F},
                       .yaw = 0.0F,
                       .pitch = getIsometricPitch()},
                   kCanvasSize,
                   halfHeight)
            .getPosition()
            .x;
    };

    EXPECT_NEAR(square(3.0F), 3.0F * pixel, 1e-5F);
    EXPECT_NEAR(square(3.0F) - square(2.0F), pixel, 1e-5F);
    EXPECT_NEAR(square(0.3F), 0.3F * pixel, 1e-5F);
}
