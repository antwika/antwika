#include <gtest/gtest.h>

#include <antwika/component/Orientation.hpp>

#include "antwika/rules/Orientation.hpp"

using antwika::component::Orientation;
using antwika::rules::getTurnedBy;
using antwika::rules::kMaxPitch;

namespace
{

    constexpr float kTolerance = 1e-5F;

}

TEST(OrientationTest, TurnedBy_CarriesTheYawRoundAndTheHeadUp)
{
    const auto turnedOrientation =
        getTurnedBy(Orientation{.yaw = 0.5F, .pitch = 0.1F}, 0.25F, 0.2F);

    EXPECT_NEAR(turnedOrientation.yaw, 0.75F, kTolerance);
    EXPECT_NEAR(turnedOrientation.pitch, 0.3F, kTolerance);
}

TEST(OrientationTest, TurnedBy_HoldsTheHeadWithinItsHighestPitch)
{
    EXPECT_NEAR(
        getTurnedBy(Orientation{}, 0.0F, kMaxPitch * 2.0F).pitch,
        kMaxPitch,
        kTolerance);
    EXPECT_NEAR(
        getTurnedBy(Orientation{}, 0.0F, -kMaxPitch * 2.0F).pitch,
        -kMaxPitch,
        kTolerance);
}
