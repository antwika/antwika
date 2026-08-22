#include <gtest/gtest.h>

#include <antwika/component/Orientation.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/rules/Orientation.hpp>

#include "antwika/system/OrientationSystem.hpp"
#include "antwika/gameplay/GameLoop.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::gameplay::Phase;
using antwika::system::OrientationSystem;
using antwika::component::Orientation;
using antwika::input::DirectionKeys;
using antwika::rules::kTurnRate;
using antwika::gameplay::GameLoop;
using antwika::rules::rotatedBy;
using antwika::rules::turnedBy;
using antwika::rules::kMaxPitch;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    constexpr float kTolerance = 0.0001F;

}

TEST(OrientationTest, RotatedBy_CarriesAnOrientationTheWayThatIsHeld)
{
    const auto orientation =
        rotatedBy(
            Orientation{},
            DirectionKeys{.north = true, .east = true});

    EXPECT_NEAR(orientation.yaw, kTurnRate, kTolerance);
    EXPECT_NEAR(orientation.pitch, -kTurnRate, kTolerance);
}

TEST(OrientationTest, RotatedBy_LeavesAnOrientationHeldBothWaysWhereItWas)
{
    const auto orientation = rotatedBy(
        Orientation{.yaw = 0.5F, .pitch = 0.25F},
        DirectionKeys{
            .north = true, .south = true, .west = true, .east = true});

    EXPECT_NEAR(orientation.yaw, 0.5F, kTolerance);
    EXPECT_NEAR(orientation.pitch, 0.25F, kTolerance);
}

TEST(OrientationTest, Update_TurnsEveryOrientationOfTheWorld)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    DirectionKeys lookKeys;
    OrientationSystem orientationSystem(lookKeys);

    gameLoop.addSystem(Phase::Orienting, orientationSystem);

    const auto entity = gameLoop.world().create();

    {
        const OpenPhase phase(gameLoop.world());

        gameLoop.world().add<Orientation>(entity, Orientation{});
    }

    lookKeys.east = true;
    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.world().get<Orientation>(entity).yaw,
        kTurnRate,
        kTolerance);
}

TEST(OrientationTest, TurnedBy_CarriesAnOrientationRoundAndTipsIt)
{
    const auto orientation =
        turnedBy(Orientation{.yaw = 0.5F, .pitch = 0.25F}, 0.1F, -0.2F);

    EXPECT_NEAR(orientation.yaw, 0.6F, kTolerance);
    EXPECT_NEAR(orientation.pitch, 0.05F, kTolerance);
}

TEST(OrientationTest, TurnedBy_TipsNoFurtherThanTheWorldStaysUpright)
{
    EXPECT_NEAR(
        turnedBy(Orientation{}, 0.0F, 90.0F).pitch,
        kMaxPitch,
        kTolerance);
    EXPECT_NEAR(
        turnedBy(Orientation{}, 0.0F, -90.0F).pitch,
        -kMaxPitch,
        kTolerance);
}

TEST(OrientationTest, TurnedBy_CarriesAnOrientationAsFarRoundAsItIsAsked)
{
    EXPECT_NEAR(
        turnedBy(Orientation{}, 90.0F, 0.0F).yaw, 90.0F, kTolerance);
}
