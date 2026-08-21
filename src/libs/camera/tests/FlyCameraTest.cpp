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
using antwika::camera::forwardOnGround;
using antwika::camera::kMaxPitch;
using antwika::camera::forward;
using antwika::camera::panned;
using antwika::camera::kPanStep;
using antwika::camera::centeredOn;
using antwika::camera::defaultTransform;
using antwika::gfx::Size;
using antwika::camera::resetToIsometric;
using antwika::camera::rotated;
using antwika::camera::movedOnGround;
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
    const auto way = forward(CameraTransform{});

    EXPECT_NEAR(way.x, 0.0F, kTolerance);
    EXPECT_NEAR(way.y, 0.0F, kTolerance);
    EXPECT_NEAR(way.z, -1.0F, kTolerance);
}

TEST(FlyCameraTest, Forward_TurnsToTheRightAsTheYawGrows)
{
    const auto way = forward(CameraTransform{.yaw = kQuarterTurn});

    EXPECT_NEAR(way.x, 1.0F, kTolerance);
    EXPECT_NEAR(way.z, 0.0F, kTolerance);
}

TEST(FlyCameraTest, Forward_DipsBelowTheHorizonOnALesserPitch)
{
    const auto way = forward(CameraTransform{.pitch = -0.5F});

    EXPECT_LT(way.y, 0.0F);
    EXPECT_NEAR(glm::length(way), 1.0F, kTolerance);
}

TEST(FlyCameraTest, ForwardOnGround_StaysLevelHoweverFarTheCameraTips)
{
    for (const float pitch : {-1.4F, -0.5F, 0.0F, 1.4F})
    {
        const auto way = forwardOnGround(
            CameraTransform{.yaw = 0.8F,
            .pitch = pitch});

        EXPECT_NEAR(way.y, 0.0F, kTolerance) << pitch;
        EXPECT_NEAR(glm::length(way), 1.0F, kTolerance) << pitch;
    }
}

TEST(FlyCameraTest, Rotated_LeavesTheCameraStandingWhereItWas)
{
    const CameraTransform transform{.position = Vec3{3.0F, 4.0F, 5.0F}};
    const auto rotatedTransform = rotated(transform, 1.2F, -0.4F);

    EXPECT_EQ(rotatedTransform.position, transform.position);
    EXPECT_NEAR(rotatedTransform.yaw, 1.2F, kTolerance);
    EXPECT_NEAR(rotatedTransform.pitch, -0.4F, kTolerance);
}

TEST(FlyCameraTest, Rotated_StopsShortOfStraightUpAndStraightDown)
{
    EXPECT_NEAR(
        rotated(CameraTransform{}, 0.0F, 100.0F).pitch,
        kMaxPitch,
        kTolerance);
    EXPECT_NEAR(
        rotated(CameraTransform{}, 0.0F, -100.0F).pitch,
        -kMaxPitch,
        kTolerance);
}

TEST(FlyCameraTest, MovedOnGround_GoesTheWayItFacesAndToItsRight)
{
    const auto ahead = movedOnGround(CameraTransform{}, 1.0F, 0.0F, 2.0F);

    EXPECT_NEAR(ahead.position.x, 0.0F, kTolerance);
    EXPECT_NEAR(ahead.position.z, -2.0F, kTolerance);

    const auto movedTransform = movedOnGround(CameraTransform{}, 0.0F, 1.0F,
        2.0F);

    EXPECT_NEAR(movedTransform.position.x, 2.0F, kTolerance);
    EXPECT_NEAR(movedTransform.position.z, 0.0F, kTolerance);
}

TEST(FlyCameraTest, MovedOnGround_KeepsToItsOwnHeightHoweverItLooks)
{
    const CameraTransform transform{
        .position = Vec3{0.0F, 5.0F, 0.0F}, .pitch = -1.2F};

    EXPECT_NEAR(
        movedOnGround(transform, 1.0F, 0.0F, 3.0F).position.y,
        5.0F,
        kTolerance);
}

TEST(FlyCameraTest, MovedOnGround_CoversTheSameGroundOnADiagonal)
{
    const auto straight = movedOnGround(CameraTransform{}, 1.0F, 0.0F, 1.0F);
    const auto corner = movedOnGround(CameraTransform{}, 1.0F, 1.0F, 1.0F);

    EXPECT_NEAR(
        glm::length(corner.position),
        glm::length(straight.position),
        kTolerance);
}

TEST(FlyCameraTest, MovedOnGround_StandsStillWhenAskedForNothing)
{
    const CameraTransform transform{.position = Vec3{1.0F, 2.0F, 3.0F}};

    EXPECT_EQ(movedOnGround(transform, 0.0F, 0.0F, 1.0F), transform);
}

TEST(FlyCameraTest, CameraOf_StandsWhereTheTransformDoesAndLooksItsWay)
{
    const auto transform = defaultTransform();
    const auto camera = cameraOf(transform, kCanvasSize, 2.4F);

    EXPECT_NEAR(camera.position().x, transform.position.x, kTolerance);
    EXPECT_NEAR(camera.position().y, transform.position.y, kTolerance);
    EXPECT_NEAR(camera.position().z, transform.position.z, kTolerance);

    const auto way =
        glm::normalize(camera.target() - camera.position());
    const auto forwardDirection = forward(transform);

    EXPECT_NEAR(way.x, forwardDirection.x, kTolerance);
    EXPECT_NEAR(way.y, forwardDirection.y, kTolerance);
    EXPECT_NEAR(way.z, forwardDirection.z, kTolerance);
}

TEST(FlyCameraTest, DefaultTransform_StandsBackAndLooksDownAtTheOrigin)
{
    const auto transform = defaultTransform();
    const auto way = forward(transform);

    EXPECT_GT(transform.position.y, 0.0F);
    EXPECT_GT(transform.position.z, 0.0F);
    EXPECT_LT(way.y, 0.0F);

    const auto ontoPoint =
        transform.position + (way * glm::length(transform.position));

    EXPECT_NEAR(glm::length(ontoPoint), 0.0F, 0.01F);
}

TEST(FlyCameraTest, Panned_MovesTheCameraWithoutTurningIt)
{
    const auto was = defaultTransform();
    const auto pannedTransform = panned(was, 3.0F, 2.0F, kPanStep);

    EXPECT_FLOAT_EQ(pannedTransform.yaw, was.yaw);
    EXPECT_FLOAT_EQ(pannedTransform.pitch, was.pitch);
    EXPECT_NE(pannedTransform.position, was.position);
}

TEST(FlyCameraTest, Panned_ComesBackWhereItBeganGoingBothWays)
{
    const auto was = defaultTransform();
    const auto pannedTransform = panned(was, 4.0F, -2.0F, kPanStep);
    const auto pannedBack = panned(pannedTransform, -4.0F, 2.0F, kPanStep);

    EXPECT_NEAR(glm::length(pannedBack.position - was.position), 0.0F, 1e-5F);
}

TEST(FlyCameraTest, Panned_SlidesAcrossTheViewAndNotAlongIt)
{
    const auto was = defaultTransform();
    const auto pannedTransform = panned(was, 5.0F, 0.0F, kPanStep);
    const auto forwardDirection = forward(was);

    EXPECT_NEAR(
        glm::dot(pannedTransform.position - was.position, forwardDirection),
        0.0F,
        1e-5F);
}

TEST(FlyCameraTest, Panned_StaysWhereItIsAskedForNothing)
{
    const auto was = defaultTransform();

    EXPECT_EQ(panned(was, 0.0F, 0.0F, kPanStep), was);
}

TEST(FlyCameraTest, Squared_LooksForwardAndPitchedDownAgain)
{
    const auto rotatedTransform = rotated(defaultTransform(), 1.2F, 0.4F);
    const auto squaredTransform = resetToIsometric(rotatedTransform);

    EXPECT_FLOAT_EQ(squaredTransform.yaw, 0.0F);
    EXPECT_FLOAT_EQ(squaredTransform.pitch, defaultTransform().pitch);
}

TEST(FlyCameraTest, Squared_LeavesTheCameraWhereItStands)
{
    const auto movedTransform = panned(
        rotated(defaultTransform(), 1.0F, 0.2F), 9.0F, 4.0F, kPanStep);

    EXPECT_EQ(resetToIsometric(movedTransform).position,
        movedTransform.position);
}

TEST(FlyCameraTest, Squared_ChangesNothingAboutTheOpeningTransform)
{
    EXPECT_EQ(resetToIsometric(defaultTransform()), defaultTransform());
}

TEST(FlyCameraTest, CameraOf_KeepsDrawingWhatItIsFlownPast)
{
    const auto camera = cameraOf(
        defaultTransform(), Size{.width = 320, .height = 180}, 2.0F);
    const auto *ortho =
        std::get_if<antwika::gfx::Orthographic>(&camera.projection());

    ASSERT_NE(ortho, nullptr);
    EXPECT_LT(ortho->nearPlane, 0.0F);
    EXPECT_GT(ortho->farPlane, 0.0F);
}

TEST(FlyCameraTest, CameraOf_KeepsAsMuchBehindItAsBefore)
{
    const auto camera = cameraOf(
        defaultTransform(), Size{.width = 320, .height = 180}, 2.0F);
    const auto *ortho =
        std::get_if<antwika::gfx::Orthographic>(&camera.projection());

    ASSERT_NE(ortho, nullptr);
    EXPECT_FLOAT_EQ(ortho->nearPlane, -ortho->farPlane);
}

TEST(FlyCameraTest, CenteredOn_MovesTheCameraToFrameAPlace)
{
    const antwika::gfx::Vec3 awayPosition{9.0F, 0.0F, 4.0F};
    const auto transform = centeredOn(defaultTransform(), awayPosition);

    EXPECT_EQ(transform.position, defaultTransform().position + awayPosition);
    EXPECT_FLOAT_EQ(transform.yaw, defaultTransform().yaw);
    EXPECT_FLOAT_EQ(transform.pitch, defaultTransform().pitch);
}

TEST(FlyCameraTest, CenteredOn_LeavesTheDefaultTransformAloneForTheOrigin)
{
    EXPECT_EQ(
        centeredOn(defaultTransform(), antwika::gfx::Vec3{0.0F, 0.0F, 0.0F}),
        defaultTransform());
}

TEST(FlyCameraTest, SquaredPitch_IsTheAngleWhoseTangentIsFourThirds)
{
    using antwika::camera::isometricPitch;

    EXPECT_NEAR(std::tan(-isometricPitch()), 4.0F / 3.0F, 1e-6F);
    EXPECT_NEAR(std::sin(-isometricPitch()), 0.8F, 1e-6F);
    EXPECT_NEAR(std::cos(-isometricPitch()), 0.6F, 1e-6F);
}

TEST(FlyCameraTest, SnappedPitch_MakesAPitchWrittenBackSquareAgain)
{
    using antwika::camera::isometricPitch;
    using antwika::camera::snappedPitch;

    const auto roundedPitch =
        static_cast<float>(std::llround(isometricPitch() * 1000.0F))
        / 1000.0F;

    EXPECT_NE(roundedPitch, isometricPitch());
    EXPECT_EQ(
        snappedPitch(CameraTransform{.pitch = roundedPitch}).pitch,
        isometricPitch());
}

TEST(FlyCameraTest, SnappedPitch_LeavesAPitchFlownElsewhereAsItWas)
{
    using antwika::camera::snappedPitch;

    EXPECT_EQ(snappedPitch(CameraTransform{.pitch = -0.5F}).pitch, -0.5F);
    EXPECT_EQ(snappedPitch(CameraTransform{.pitch = 0.0F}).pitch, 0.0F);
}

TEST(FlyCameraTest, OrthoHalfHeight_DrawsAVoxelTheAskedPixelsAcross)
{
    using antwika::camera::kCanvasSize;
    using antwika::camera::kVoxelPixels;
    using antwika::camera::orthoHalfHeight;

    for (const std::int32_t pixelsPerVoxel :
         {kVoxelPixels, kVoxelPixels * 2, kVoxelPixels * 3})
    {
        const auto halfHeight =
            orthoHalfHeight(kCanvasSize, pixelsPerVoxel);
        const auto pixels = static_cast<float>(kCanvasSize.height)
                            / (2.0F * halfHeight);

        EXPECT_NEAR(pixels, static_cast<float>(pixelsPerVoxel), 1e-3F);
    }
}

TEST(FlyCameraTest, CameraOf_StandsWhereTheTransformStands)
{
    using antwika::camera::kCanvasSize;
    using antwika::camera::kVoxelPixels;
    using antwika::camera::isometricPitch;
    using antwika::camera::orthoHalfHeight;

    const auto halfHeight =
        orthoHalfHeight(kCanvasSize, kVoxelPixels);
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
                       .pitch = isometricPitch()},
                   kCanvasSize,
                   halfHeight)
            .position()
            .x;
    };

    EXPECT_NEAR(square(3.0F), 3.0F * pixel, 1e-5F);
    EXPECT_NEAR(square(3.0F) - square(2.0F), pixel, 1e-5F);
    EXPECT_NEAR(square(0.3F), 0.3F * pixel, 1e-5F);
}
