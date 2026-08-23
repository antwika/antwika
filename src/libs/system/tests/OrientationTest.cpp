#include <gtest/gtest.h>

#include <antwika/component/TurnIntent.hpp>
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
using antwika::intent::DirectionKeys;
using antwika::rules::kTurnRate;
using antwika::gameplay::GameLoop;
using antwika::rules::getRotatedBy;
using antwika::rules::getTurnedBy;
using antwika::rules::kMaxPitch;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    constexpr float kTolerance = 0.0001F;

    [[nodiscard]] antwika::component::TurnIntent intentOf(
        const DirectionKeys keys)
    {
        return antwika::component::TurnIntent{
            .axisX = keys.getAxisX(), .axisZ = keys.getAxisZ()};
    }

}

TEST(OrientationTest, RotatedBy_CarriesAnOrientationTheWayThatIsHeld)
{
    const auto orientation =
        getRotatedBy(
            Orientation{},
            intentOf(DirectionKeys{.north = true, .east = true}));

    EXPECT_NEAR(orientation.yaw, kTurnRate, kTolerance);
    EXPECT_NEAR(orientation.pitch, -kTurnRate, kTolerance);
}

TEST(OrientationTest, RotatedBy_LeavesAnOrientationHeldBothWaysWhereItWas)
{
    const auto orientation = getRotatedBy(
        Orientation{.yaw = 0.5F, .pitch = 0.25F},
        intentOf(
            DirectionKeys{
                .north = true,
                .south = true,
                .west = true,
                .east = true}));

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

    const auto entity = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<Orientation>(entity, Orientation{});
    }

    lookKeys.east = true;
    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.getWorld().get<Orientation>(entity).yaw,
        kTurnRate,
        kTolerance);
}

TEST(OrientationTest, TurnedBy_CarriesAnOrientationRoundAndTipsIt)
{
    const auto orientation =
        getTurnedBy(Orientation{.yaw = 0.5F, .pitch = 0.25F}, 0.1F, -0.2F);

    EXPECT_NEAR(orientation.yaw, 0.6F, kTolerance);
    EXPECT_NEAR(orientation.pitch, 0.05F, kTolerance);
}

TEST(OrientationTest, TurnedBy_TipsNoFurtherThanTheWorldStaysUpright)
{
    EXPECT_NEAR(
        getTurnedBy(Orientation{}, 0.0F, 90.0F).pitch,
        kMaxPitch,
        kTolerance);
    EXPECT_NEAR(
        getTurnedBy(Orientation{}, 0.0F, -90.0F).pitch,
        -kMaxPitch,
        kTolerance);
}

TEST(OrientationTest, TurnedBy_CarriesAnOrientationAsFarRoundAsItIsAsked)
{
    EXPECT_NEAR(
        getTurnedBy(Orientation{}, 90.0F, 0.0F).yaw, 90.0F, kTolerance);
}
