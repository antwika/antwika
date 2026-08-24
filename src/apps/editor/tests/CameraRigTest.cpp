#include <gtest/gtest.h>

#include <cmath>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/editor/editor/CameraRig.hpp>
#include <antwika/input/Position.hpp>

using antwika::editor::CameraRig;

namespace
{
    constexpr float kSlack = 0.001F;

    [[nodiscard]] CameraRig getRigLookingAhead()
    {
        CameraRig rig;

        rig.viewHeight = 10.0F;
        rig.view.transform.position = antwika::gfx::Vec3{0.0F, 0.0F, 0.0F};

        return rig;
    }
}

TEST(CameraRigTest, Orbit_TurnsTheViewItLooksAlong)
{
    auto rig = getRigLookingAhead();
    const auto wasForward =
        antwika::camera::getForward(rig.view.transform);

    rig.orbit(0.5F, 0.0F);

    const auto isForward = antwika::camera::getForward(rig.view.transform);

    EXPECT_GT(
        std::abs(isForward.x - wasForward.x)
            + std::abs(isForward.z - wasForward.z),
        kSlack);
}

TEST(CameraRigTest, Orbit_KeepsTheEyeWhereItStood)
{
    auto rig = getRigLookingAhead();
    const auto backDistance =
        rig.viewHeight / std::tan(antwika::camera::kEditorFov / 2.0F);
    const auto wasEye =
        rig.view.transform.position
        - (antwika::camera::getForward(rig.view.transform) * backDistance);

    rig.orbit(0.4F, 0.2F);

    const auto isEye =
        rig.view.transform.position
        - (antwika::camera::getForward(rig.view.transform) * backDistance);

    EXPECT_NEAR(isEye.x, wasEye.x, kSlack);
    EXPECT_NEAR(isEye.y, wasEye.y, kSlack);
    EXPECT_NEAR(isEye.z, wasEye.z, kSlack);
}

TEST(CameraRigTest, Orbit_LeavesTheViewWhereItStoodForNoTurn)
{
    auto rig = getRigLookingAhead();
    const auto wasPosition = rig.view.transform.position;

    rig.orbit(0.0F, 0.0F);

    EXPECT_NEAR(rig.view.transform.position.x, wasPosition.x, kSlack);
    EXPECT_NEAR(rig.view.transform.position.y, wasPosition.y, kSlack);
    EXPECT_NEAR(rig.view.transform.position.z, wasPosition.z, kSlack);
}

TEST(CameraRigTest, DragOrbit_WaitsForTheHandToLeaveWhereItPressed)
{
    auto rig = getRigLookingAhead();

    rig.orbitFromPosition = antwika::input::Position{.x = 100, .y = 100};
    rig.dragOrbit(
        antwika::input::Position{.x = 101, .y = 101},
        antwika::input::Position{.x = 100, .y = 100});

    EXPECT_FALSE(rig.orbiting);
}

TEST(CameraRigTest, DragOrbit_TakesUpTheOrbitOnceTheHandHasMoved)
{
    auto rig = getRigLookingAhead();

    rig.orbitFromPosition = antwika::input::Position{.x = 100, .y = 100};
    rig.dragOrbit(
        antwika::input::Position{.x = 120, .y = 100},
        antwika::input::Position{.x = 100, .y = 100});

    EXPECT_TRUE(rig.orbiting);
}

TEST(CameraRigTest, DragOrbit_LeavesTheViewAloneWithNoPressToOrbitFrom)
{
    auto rig = getRigLookingAhead();
    const auto wasTransform = rig.view.transform;

    rig.dragOrbit(
        antwika::input::Position{.x = 200, .y = 200},
        antwika::input::Position{.x = 100, .y = 100});

    EXPECT_FALSE(rig.orbiting);
    EXPECT_NEAR(rig.view.transform.position.x, wasTransform.position.x, kSlack);
}

TEST(CameraRigTest, DragOrbit_KeepsTurningOnceItHasTakenTheOrbitUp)
{
    auto rig = getRigLookingAhead();

    rig.orbitFromPosition = antwika::input::Position{.x = 100, .y = 100};
    rig.orbiting = true;

    const auto wasForward =
        antwika::camera::getForward(rig.view.transform);

    rig.dragOrbit(
        antwika::input::Position{.x = 101, .y = 100},
        antwika::input::Position{.x = 100, .y = 100});

    const auto isForward = antwika::camera::getForward(rig.view.transform);

    EXPECT_GT(
        std::abs(isForward.x - wasForward.x)
            + std::abs(isForward.z - wasForward.z),
        0.0F);
}

