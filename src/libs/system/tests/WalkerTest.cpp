#include <gtest/gtest.h>

#include <cmath>
#include <set>
#include <vector>

#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/system/WalkerSystems.hpp"
#include "antwika/gameplay/GameLoop.hpp"

using antwika::ecs::World;
using antwika::voxel::Voxels;
using antwika::gameplay::Phase;
using antwika::collision::isSolid;
using antwika::collision::spawnPosition;
using antwika::collision::restPositionOverColumn;
using antwika::collision::groundHeightAtColumn;
using antwika::voxel::Kind;
using antwika::voxel::kVoxelSide;
using antwika::collision::kWalkSpeed;
using antwika::collision::positionOf;
using antwika::collision::stoodCells;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::component::Player;
using antwika::collision::hasHeadroom;
using antwika::system::MoveIntentSystem;
using antwika::gameplay::GameLoop;
using antwika::component::Position;
using antwika::collision::positionFrom;
using antwika::voxel::VoxelCell;
using antwika::collision::movedWithCollision;
using antwika::component::Velocity;
using antwika::system::WalkSystem;
using antwika::log::mocks::MockLogger;
using antwika::voxel::VoxelPosition;
using ::testing::NiceMock;

namespace
{

    constexpr float kTolerance = 0.0001F;

    constexpr float kTread =
        kVoxelSide
        / static_cast<float>(antwika::voxel::kStepsPerVoxel);

    [[nodiscard]] Voxels floorOver(
        const std::int32_t reach)
    {
        Voxels voxels;

        for (auto x = -reach; x <= reach; ++x)
        {
            for (auto z = -reach; z <= reach; ++z)
            {
                voxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
                    .z = z}}}));
            }
        }

        return voxels;
    }

    [[nodiscard]] Voxels filledOver(
        const std::int32_t reach)
    {
        const auto voxels = floorOver(reach);

        return Voxels(voxels.begin(), voxels.end());
    }

}

TEST(WalkerTest, PositionFrom_TakesBackWhatPositionOfGave)
{
    const antwika::gfx::Vec3 position{3.25F, 6.5F, -2.125F};
    const auto stoodPosition = positionFrom(position);

    EXPECT_FLOAT_EQ(stoodPosition.x, position.x);
    EXPECT_FLOAT_EQ(stoodPosition.y, position.y);
    EXPECT_FLOAT_EQ(stoodPosition.z, position.z);
    EXPECT_FLOAT_EQ(positionOf(stoodPosition).x, position.x);
    EXPECT_FLOAT_EQ(positionOf(stoodPosition).y, position.y);
    EXPECT_FLOAT_EQ(positionOf(stoodPosition).z, position.z);
}

TEST(WalkerTest, IsSolid_TakesStoneAndStairsAndNotWaterOrAir)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 2, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}}});

    EXPECT_TRUE(isSolid(filledVoxels, VoxelPosition{.x = 0, .y = 0, .z = 0}));
    EXPECT_TRUE(isSolid(filledVoxels, VoxelPosition{.x = 1, .y = 0, .z = 0}));
    EXPECT_FALSE(isSolid(filledVoxels, VoxelPosition{.x = 2, .y = 0, .z = 0}));
    EXPECT_FALSE(isSolid(filledVoxels, VoxelPosition{.x = 3, .y = 0, .z = 0}));
}

TEST(WalkerTest, HasHeadroom_IsDeniedByWhateverStandsWithinAWalker)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 2, .z = 0}}});

    EXPECT_FALSE(hasHeadroom(filledVoxels, VoxelPosition{.x = 0, .y = 0,
        .z = 0}));
    EXPECT_TRUE(hasHeadroom(filledVoxels, VoxelPosition{.x = 0, .y = 2,
        .z = 0}));
}

TEST(WalkerTest, GroundHeightAtColumn_GivesTheTopOfWhatBearsUnderneath)
{
    const auto filledVoxels = filledOver(1);
    const auto footing = groundHeightAtColumn(filledVoxels, 0, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 1.0F, kTolerance);
}

TEST(WalkerTest, GroundHeightAtColumn_TakesAStepUpOfOneVoxel)
{
    auto filledVoxels = filledOver(1);
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));

    const auto footing = groundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 2.0F, kTolerance);
}

TEST(WalkerTest, GroundHeightAtColumn_TakesAStepDownOfOneVoxel)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = -1, .z = 0}}});

    const auto footing = groundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 0.0F, kTolerance);
}

TEST(WalkerTest, GroundHeightAtColumn_DropsToGroundFarBelow)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = -2, .z = 0}}});

    const auto footing = groundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, -1.0F, kTolerance);
    EXPECT_FALSE(groundHeightAtColumn(filledVoxels, 2, 0, 1.0F).has_value());
}

TEST(WalkerTest, GroundHeightAtColumn_GivesUpPastTheMaxFallDepth)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1,
            .y = -antwika::collision::kMaxFallDepth - 2, .z = 0}}});

    EXPECT_FALSE(groundHeightAtColumn(filledVoxels, 1, 0, 1.0F).has_value());
}

TEST(WalkerTest, GroundHeightAtColumn_StandsInWaterSunkToTheWaist)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}}});

    const auto footing = groundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 0.5F, kTolerance);
}

TEST(WalkerTest, MovedWithCollision_FallsNoFasterThanTheFallSpeed)
{
    Voxels filledVoxels;

    for (std::int32_t x = 0; x <= 8; ++x)
    {
        filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
            .z = 0}}}));
        filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
            .z = 1}}}));
        filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
            .z = -1}}}));
    }

    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = 1,
        .z = 0}}}));
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = 2,
        .z = 0}}}));
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = 3,
        .z = 0}}}));

    auto stoodPosition = Position{.x = 0.5F, .y = 4.0F, .z = 0.5F};
    auto sankMost = 0.0F;

    for (std::size_t step = 0; step < 120; ++step)
    {
        const auto was = stoodPosition;

        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F,
                .velocityZ = 0.0F});
        sankMost = std::max(sankMost, was.y - stoodPosition.y);
    }

    EXPECT_NEAR(stoodPosition.y, 1.0F, kTolerance);
    EXPECT_GT(stoodPosition.x, 2.0F);
    EXPECT_LE(
        sankMost, antwika::collision::kFallSpeed + kTolerance);
}

TEST(WalkerTest, MovedWithCollision_SinksWhereItStandsOverAHole)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = -3, .z = 0}}});

    auto stoodPosition = Position{.x = 0.5F, .y = 3.0F, .z = 0.5F};

    for (std::size_t step = 0; step < 60; ++step)
    {
        stoodPosition = movedWithCollision(filledVoxels, stoodPosition,
            Velocity{});
    }

    EXPECT_NEAR(stoodPosition.y, -2.0F, kTolerance);
}

TEST(WalkerTest, MovedWithCollision_WadesAtTheWadersPace)
{
    Voxels landVoxels;
    Voxels poolVoxels;

    for (std::int32_t x = -4; x <= 4; ++x)
    {
        for (std::int32_t z = -1; z <= 1; ++z)
        {
            landVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
                .z = z}}}));
            poolVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
                .z = z}, .material = {.kind = Kind::Water}}}));
        }
    }

    auto dryPosition = Position{.x = -1.5F, .y = 1.0F, .z = 0.5F};
    auto wetPosition = Position{.x = -1.5F, .y = 0.5F, .z = 0.5F};

    for (std::size_t step = 0; step < 10; ++step)
    {
        dryPosition = movedWithCollision(landVoxels, dryPosition,
        Velocity{.velocityX = 1.0F});
        wetPosition = movedWithCollision(poolVoxels, wetPosition,
            Velocity{.velocityX = 1.0F});
    }

    EXPECT_NEAR(
        (wetPosition.x + 1.5F) / (dryPosition.x + 1.5F),
        antwika::collision::kWaterSpeedFactor,
        0.01F);
}

TEST(WalkerTest, MovedWithCollision_ClimbsALadderNorthAndDownAgain)
{
    auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1}}});

    for (std::int32_t y = 1; y <= 4; ++y)
    {
        filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = y,
            .z = 0}, .material = {.kind = Kind::Ladder}}}));
    }

    auto stoodPosition = Position{.x = 0.5F, .y = 1.0F, .z = 0.5F};

    for (std::size_t step = 0; step < 200; ++step)
    {
        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityZ = -1.0F});
    }

    EXPECT_NEAR(stoodPosition.y, 5.0F, kTolerance);
    EXPECT_LT(std::abs(stoodPosition.z - 0.5F), 0.3F);

    for (std::size_t step = 0; step < 200; ++step)
    {
        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityZ = 1.0F});
    }

    EXPECT_NEAR(stoodPosition.y, 1.0F, 0.2F);
}

TEST(WalkerTest, MovedWithCollision_StepsOffTheTopOfALadder)
{
    auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}}});

    for (std::int32_t y = 1; y <= 3; ++y)
    {
        filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = y,
            .z = 0}, .material = {.kind = Kind::Ladder}}}));
    }

    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = 3,
        .z = -1}}}));
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = 3,
        .z = -2}}}));

    auto stoodPosition = Position{.x = 0.5F, .y = 1.0F, .z = 0.5F};

    for (std::size_t step = 0; step < 400; ++step)
    {
        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityZ = -1.0F});
    }

    EXPECT_NEAR(stoodPosition.y, 4.0F, kTolerance);
    EXPECT_LT(stoodPosition.z, -0.4F);
}

TEST(WalkerTest, MovedWithCollision_HoldsToTheRungsWithoutFalling)
{
    auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = -4, .z = 0}}});

    for (std::int32_t y = 0; y <= 3; ++y)
    {
        filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = y,
            .z = 0}, .material = {.kind = Kind::Ladder}}}));
    }

    auto stoodPosition = Position{.x = 0.5F, .y = 2.5F, .z = 0.5F};

    for (std::size_t step = 0; step < 30; ++step)
    {
        stoodPosition = movedWithCollision(filledVoxels, stoodPosition,
            Velocity{});
    }

    EXPECT_NEAR(stoodPosition.y, 2.5F, kTolerance);
}

TEST(WalkerTest, GroundHeightAtColumn_FallsPastALadderItDoesNotHold)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = -3, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ladder}}});

    const auto footing = groundHeightAtColumn(filledVoxels, 0, 0, 3.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, -2.0F, kTolerance);
}

TEST(WalkerTest, MovedWithCollision_RunsByTheMultiplierItIsSent)
{
    Voxels filledVoxels;

    for (std::int32_t x = -8; x <= 8; ++x)
    {
        for (std::int32_t z = -1; z <= 1; ++z)
        {
            filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
                .z = z}}}));
        }
    }

    auto walkedPosition = Position{.x = -3.5F, .y = 1.0F, .z = 0.5F};
    auto ran = Position{.x = -3.5F, .y = 1.0F, .z = 0.5F};

    for (std::size_t step = 0; step < 10; ++step)
    {
        walkedPosition =
            movedWithCollision(
                filledVoxels, walkedPosition, Velocity{.velocityX = 1.0F});
        ran = movedWithCollision(
            filledVoxels,
            ran,
            Velocity{
                .velocityX = 1.0F,
                .speedMultiplier = antwika::collision::kRunSpeedMultiplier});
    }

    EXPECT_NEAR(
        (ran.x + 3.5F) / (walkedPosition.x + 3.5F),
        antwika::collision::kRunSpeedMultiplier,
        0.01F);
}

TEST(WalkerTest, GroundHeightAtColumn_RefusesGroundWithNoRoomOverIt)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 1, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 2, .z = 0}}});

    EXPECT_FALSE(groundHeightAtColumn(filledVoxels, 1, 0, 1.0F).has_value());
}

TEST(WalkerTest, RestPositionOverColumn_TakesTheHighestOfAColumnWithRoom)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 1, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 4, .z = 0}}});

    const auto foothold = restPositionOverColumn(filledVoxels, 0, 0);

    ASSERT_TRUE(foothold.has_value());
    EXPECT_NEAR(foothold->y, 5.0F, kTolerance);
}

TEST(WalkerTest, RestPositionOverColumn_GivesNothingOverAnEmptyColumn)
{
    EXPECT_FALSE(restPositionOverColumn(filledOver(1), 8, 8).has_value());
}

TEST(WalkerTest, SpawnPosition_GivesNothingOnAPileThatBearsNothing)
{
    EXPECT_FALSE(spawnPosition(Voxels{}).has_value());
}

TEST(WalkerTest, SpawnPosition_StandsOnThePile)
{
    const auto foothold = spawnPosition(floorOver(2));

    ASSERT_TRUE(foothold.has_value());
    EXPECT_NEAR(foothold->y, 1.0F, kTolerance);
    EXPECT_NEAR(foothold->x, 0.0F, kTolerance);
    EXPECT_NEAR(foothold->z, 0.0F, kTolerance);
}

TEST(WalkerTest, MovedWithCollision_CarriesAWalkerAcrossTheGround)
{
    const auto filledVoxels = filledOver(2);
    const auto stoodPosition = movedWithCollision(
        filledVoxels,
        Position{.x = 0.5F, .y = 1.0F, .z = 0.5F},
        Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});

    EXPECT_NEAR(stoodPosition.x, 0.5F + kWalkSpeed, kTolerance);
    EXPECT_NEAR(stoodPosition.y, 1.0F, kTolerance);
    EXPECT_NEAR(stoodPosition.z, 0.5F, kTolerance);
}

TEST(WalkerTest, MovedWithCollision_CrossesARampAtHalfItsPace)
{
    auto filledVoxels = filledOver(2);

    filledVoxels.erase(VoxelPosition{.x = 0, .y = 0, .z = 0});
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
        .material = {.kind = Kind::Ramp}}}));

    const auto stoodPosition = movedWithCollision(
        filledVoxels,
        Position{.x = 0.5F, .y = 1.0F, .z = 0.5F},
        Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});

    EXPECT_NEAR(
        stoodPosition.x,
        0.5F + (kWalkSpeed * antwika::collision::kRampSpeedFactor),
        kTolerance);
}

TEST(WalkerTest, MovedWithCollision_LeavesAWalkerSentNowhereWhereItStood)
{
    const auto filledVoxels = filledOver(2);
    const Position stoodPosition{.x = 0.75F, .y = 1.0F, .z = 0.25F};
    const auto walkedTo = movedWithCollision(filledVoxels, stoodPosition,
        Velocity{});

    EXPECT_NEAR(walkedTo.x, stoodPosition.x, kTolerance);
    EXPECT_NEAR(walkedTo.y, stoodPosition.y, kTolerance);
    EXPECT_NEAR(walkedTo.z, stoodPosition.z, kTolerance);
}

TEST(WalkerTest, MovedWithCollision_ClimbsAStepOfOneVoxel)
{
    auto filledVoxels = filledOver(2);
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));

    const auto stoodPosition = movedWithCollision(
        filledVoxels,
        Position{.x = 0.95F, .y = 1.0F, .z = 0.5F},
        Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});

    EXPECT_NEAR(stoodPosition.y, 2.0F, kTolerance);
}

TEST(WalkerTest, MovedWithCollision_IsStoppedByASideTallerThanAStep)
{
    auto filledVoxels = filledOver(2);
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 2,
        .z = 0}}}));

    const Position stoodPosition{.x = 0.95F, .y = 1.0F, .z = 0.5F};
    const auto walkedTo =
        movedWithCollision(
            filledVoxels,
            stoodPosition,
            Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});

    EXPECT_NEAR(walkedTo.x, stoodPosition.x, kTolerance);
    EXPECT_NEAR(walkedTo.y, stoodPosition.y, kTolerance);
}

TEST(
    WalkerTest,
    MovedWithCollision_RunsAlongWhatHoldsItRatherThanStoppingDead)
{
    auto filledVoxels = filledOver(4);

    for (const auto alongIndex : {0, 1})
    {
        for (const auto upLevel : {1, 2})
        {
            filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1,
                .y = upLevel, .z = alongIndex}}}));
        }
    }

    auto stoodPosition = Position{.x = 0.5F, .y = 1.0F, .z = 0.5F};

    for (auto step = 0; step < 10; ++step)
    {
        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F,
                .velocityZ = 1.0F});
    }

    EXPECT_LT(
        stoodPosition.x,
        1.0F - (antwika::collision::kFootprintWidth / 2.0F) + kTolerance);
    EXPECT_GT(stoodPosition.z, 0.5F);
}

TEST(WalkerTest, MovedWithCollision_KeepsOneWayAboutFromOutrunningTwo)
{
    const auto filledVoxels = filledOver(4);
    const Position stoodPosition{.x = 0.5F, .y = 1.0F, .z = 0.5F};
    const auto walkedTo =
        movedWithCollision(
            filledVoxels,
            stoodPosition,
            Velocity{.velocityX = 1.0F, .velocityZ = 1.0F});
    const auto wentX = walkedTo.x - stoodPosition.x;
    const auto wentZ = walkedTo.z - stoodPosition.z;
    const auto went = std::sqrt((wentX * wentX) + (wentZ * wentZ));

    EXPECT_NEAR(went, kWalkSpeed, kTolerance);
}

TEST(
    WalkerTest,
    MovedWithCollision_SettlesAWalkerLeftOverGroundThatFellAway)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = -1, .z = 0}}});
    auto stoodPosition = Position{.x = 0.5F, .y = 1.0F, .z = 0.5F};

    for (std::size_t step = 0; step < 10; ++step)
    {
        stoodPosition = movedWithCollision(filledVoxels, stoodPosition,
            Velocity{});
    }

    EXPECT_NEAR(stoodPosition.y, 0.0F, kTolerance);
}

TEST(WalkerTest, Update_SendsAPlayerTheWayThatIsHeld)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    antwika::input::DirectionKeys wasdKeys;
    antwika::input::DirectionKeys arrowKeys;
    MoveIntentSystem sendingSystem(wasdKeys, arrowKeys);

    gameLoop.addSystem(Phase::Sending, sendingSystem);

    const auto entity = gameLoop.world().create();

    gameLoop.world().add<Player>(entity, Player{});
    gameLoop.world().add<Velocity>(entity, Velocity{});
    gameLoop.world().commit();

    wasdKeys.east = true;
    wasdKeys.north = true;
    gameLoop.run(0);

    const auto walkVelocity = gameLoop.world().get<Velocity>(entity);

    EXPECT_NEAR(walkVelocity.velocityX, 1.0F, kTolerance);
    EXPECT_NEAR(walkVelocity.velocityZ, -1.0F, kTolerance);
}

TEST(WalkerTest, Update_LeavesACharacterWithNoPlayerTagUnsent)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    antwika::input::DirectionKeys wasdKeys;
    antwika::input::DirectionKeys arrowKeys;
    MoveIntentSystem sendingSystem(wasdKeys, arrowKeys);

    gameLoop.addSystem(Phase::Sending, sendingSystem);

    const auto entity = gameLoop.world().create();

    gameLoop.world().add<Velocity>(entity, Velocity{});
    gameLoop.world().commit();

    wasdKeys.east = true;
    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.world().get<Velocity>(entity).velocityX,
        0.0F,
        kTolerance);
}

TEST(WalkerTest, Update_WalksTwoWalkersOverThePileAtOnce)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    auto filledVoxels = filledOver(2);
    WalkSystem steppingSystem(filledVoxels);

    gameLoop.addSystem(Phase::Walking, steppingSystem);

    const auto east = gameLoop.world().create();
    const auto west = gameLoop.world().create();

    gameLoop.world().add<Position>(
        east, Position{.x = 0.5F, .y = 1.0F, .z = 0.5F});
    gameLoop.world().add<Velocity>(
        east, Velocity{.velocityX = 1.0F});
    gameLoop.world().add<Position>(
        west, Position{.x = 0.5F, .y = 1.0F, .z = 0.5F});
    gameLoop.world().add<Velocity>(
        west, Velocity{.velocityX = -1.0F});
    gameLoop.world().commit();

    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.world().get<Position>(east).x,
        0.5F + kWalkSpeed,
        kTolerance);
    EXPECT_NEAR(
        gameLoop.world().get<Position>(west).x,
        0.5F - kWalkSpeed,
        kTolerance);
}

TEST(WalkerTest, Update_WalksEveryWalkerOverThePileAsItStands)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    auto filledVoxels = filledOver(2);
    WalkSystem steppingSystem(filledVoxels);

    gameLoop.addSystem(Phase::Walking, steppingSystem);

    const auto entity = gameLoop.world().create();

    gameLoop.world().add<Position>(
        entity, Position{.x = 0.5F, .y = 1.0F, .z = 0.5F});
    gameLoop.world().add<Velocity>(
        entity, Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});
    gameLoop.world().commit();

    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.world().get<Position>(entity).x,
        0.5F + kWalkSpeed,
        kTolerance);
}

TEST(WalkerTest, MovedWithCollision_ClimbsAFlightLaidAsARampCube)
{
    const auto filledVoxels = antwika::voxel::withBlockAt(
        Voxels{},
        VoxelPosition{.x = 0, .y = 0, .z = 0},
        Kind::Ramp,
        antwika::voxel::Facing::East);

    const auto lowGround = groundHeightAtColumn(filledVoxels, 0, 0, 1.0F);
    const auto highGround = groundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(lowGround.has_value());
    ASSERT_TRUE(highGround.has_value());
    EXPECT_NEAR(*lowGround, 1.0F, kTolerance);
    EXPECT_NEAR(*highGround, 2.0F, kTolerance);

    auto stoodPosition = Position{.x = 0.5F, .y = *lowGround, .z = 0.5F};

    for (auto step = 0; step < 20; ++step)
    {
        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F,
                .velocityZ = 0.0F});
    }

    EXPECT_GT(stoodPosition.x, 0.5F);
    EXPECT_GT(stoodPosition.y, *lowGround);
    EXPECT_LE(stoodPosition.y, *highGround);

    const auto underCell =
        antwika::collision::supportingVoxel(filledVoxels, 1, 0,
            stoodPosition.y);

    ASSERT_TRUE(underCell.has_value());
    EXPECT_NEAR(
        stoodPosition.y,
        antwika::collision::groundHeightOn(filledVoxels, *underCell,
        stoodPosition.x,
            stoodPosition.z),
        kTolerance);
}

TEST(
    WalkerTest,
    GroundHeightUnderFootprint_HoldsEveryColumnItsFootprintCovers)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 1}}});

    EXPECT_TRUE(
        antwika::collision::groundHeightUnderFootprint(
            filledVoxels,
            0.5F,
            0.5F,
            1.0F)
            .has_value());
    EXPECT_FALSE(
        antwika::collision::groundHeightUnderFootprint(
            filledVoxels,
            2.0F,
            0.5F,
            1.0F)
            .has_value());
}

TEST(
    WalkerTest,
    GroundHeightUnderFootprint_RestsOnTheHighestUnderItsFootprint)
{
    auto filledVoxels = filledOver(2);
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));

    const auto footing =
        antwika::collision::groundHeightUnderFootprint(
            filledVoxels, 1.1F, 0.5F, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 2.0F, kTolerance);
}

TEST(
    WalkerTest,
    MovedWithCollision_KeepsAWalkersFootprintOffTheEdgeOfADrop)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = -1}}});

    auto stoodPosition = Position{.x = 0.5F, .y = 1.0F, .z = 0.5F};

    for (auto step = 0; step < 20; ++step)
    {
        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F,
                .velocityZ = 0.0F});
    }

    EXPECT_GT(stoodPosition.x, 0.0F);
    EXPECT_LT(
        stoodPosition.x,
        1.0F - (antwika::collision::kFootprintWidth / 2.0F) + kTolerance);
}

TEST(WalkerTest, KWalkerPixel_MeasuresTheFootprintInWholePixels)
{
    using antwika::collision::kFootprintDepth;
    using antwika::collision::kFootprintPivotY;
    using antwika::collision::kFootprintWidth;
    using antwika::collision::kWalkerPixel;

    EXPECT_NEAR(kWalkerPixel * 12.0F, kVoxelSide, kTolerance);
    EXPECT_NEAR(kFootprintWidth / kWalkerPixel, 8.0F, kTolerance);
    EXPECT_NEAR(kFootprintDepth / kWalkerPixel, 6.0F, kTolerance);
    EXPECT_NEAR(kFootprintPivotY / kWalkerPixel, 7.0F, kTolerance);
}

namespace
{

    [[nodiscard]] Voxels rampEastward()
    {
        Voxels filledVoxels;

        for (std::int32_t z = -2; z <= 2; ++z)
        {
            for (std::int32_t x = -3; x <= 0; ++x)
            {
                filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x,
                    .y = 0, .z = z}}}));
            }

            filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
                .z =
                    z}, .material = {.kind =
                        Kind::Ramp, .facing = antwika::voxel::Facing::East}}}));
            filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 0,
                .z = z}}}));

            for (std::int32_t x = 2; x <= 4; ++x)
            {
                filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x,
                    .y = 0, .z = z}}}));
                filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x,
                    .y = 1, .z = z}}}));
            }
        }

        return filledVoxels;
    }

    [[nodiscard]] Voxels walledRamp()
    {
        auto filledVoxels = rampEastward();

        for (std::int32_t x = -3; x <= 4; ++x)
        {
            filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 1,
                .z = 3}}}));
            filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 2,
                .z = 3}}}));
        }

        return filledVoxels;
    }

}

TEST(WalkerTest, GroundHeightOn_StandsAPlainVoxelAtItsTopThroughout)
{
    const auto filledVoxels = filledOver(2);
    const VoxelCell groundCell{.position = {.x = 0, .y = 0, .z = 0}};

    for (const auto index : {0.1F, 0.5F, 0.9F})
    {
        EXPECT_NEAR(
            antwika::collision::groundHeightOn(filledVoxels, groundCell, index,
            index),
            1.0F * kVoxelSide,
            kTolerance);
    }
}

TEST(WalkerTest, GroundHeightOn_RisesEvenlyAcrossARamp)
{
    const auto filledVoxels = rampEastward();
    const VoxelCell rampCell{
        .position = {.x = 1, .y = 1, .z = 0},
        .material = {
            .kind = Kind::Ramp,
            .facing = antwika::voxel::Facing::East}};

    EXPECT_NEAR(
        antwika::collision::groundHeightOn(filledVoxels, rampCell, 1.0F, 0.5F),
        1.0F,
        kTolerance);
    EXPECT_NEAR(
        antwika::collision::groundHeightOn(filledVoxels, rampCell, 1.5F, 0.5F),
        1.5F,
        kTolerance);
    EXPECT_NEAR(
        antwika::collision::groundHeightOn(filledVoxels, rampCell, 2.0F, 0.5F),
        2.0F,
        kTolerance);
}

TEST(WalkerTest, GroundHeightOn_HoldsToTheEndsOfARampBeyondIt)
{
    const auto filledVoxels = rampEastward();
    const VoxelCell rampCell{
        .position = {.x = 1, .y = 1, .z = 0},
        .material = {
            .kind = Kind::Ramp,
            .facing = antwika::voxel::Facing::East}};

    EXPECT_NEAR(
        antwika::collision::groundHeightOn(filledVoxels, rampCell, -1.5F, 0.5F),
        1.0F,
        kTolerance);
    EXPECT_NEAR(
        antwika::collision::groundHeightOn(filledVoxels, rampCell, 4.5F, 0.5F),
        2.0F,
        kTolerance);
}

TEST(WalkerTest, MovedWithCollision_LiftsAWalkerByATreadAndNoMoreAtOnce)
{
    const auto filledVoxels = rampEastward();
    Position stoodPosition{.x = -0.5F, .y = 1.0F, .z = 0.5F};
    auto liftedHeight = 0.0F;
    auto didClimb = false;

    for (std::size_t step = 0; step < 60; ++step)
    {
        const auto was = stoodPosition;

        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F});
        liftedHeight = std::max(liftedHeight,
            std::abs(stoodPosition.y - was.y));
        didClimb = didClimb || stoodPosition.y > was.y;
    }

    EXPECT_TRUE(didClimb);
    EXPECT_NEAR(stoodPosition.y, 2.0F, kTolerance);
    EXPECT_LE(liftedHeight, kTread + kWalkSpeed + kTolerance);
}

TEST(WalkerTest, MovedWithCollision_DropsAWalkerByATreadAndNoMoreAtOnce)
{
    const auto filledVoxels = rampEastward();
    Position stoodPosition{.x = 3.5F, .y = 2.0F, .z = 0.5F};
    auto droppedHeight = 0.0F;

    for (std::size_t step = 0; step < 60; ++step)
    {
        const auto was = stoodPosition;

        stoodPosition = movedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = -1.0F});
        droppedHeight = std::max(droppedHeight,
            std::abs(stoodPosition.y - was.y));
    }

    EXPECT_NEAR(stoodPosition.y, 1.0F, kTolerance);
    EXPECT_LE(droppedHeight, kTread + kWalkSpeed + kTolerance);
}

TEST(
    WalkerTest,
    GroundHeightUnderFootprint_StandsWhereEveryColumnBearsItStill)
{
    const auto filledVoxels = walledRamp();

    for (auto index = 0; index <= 60; ++index)
    {
        const auto x = -1.0F + (static_cast<float>(index) * 0.05F);

        for (auto alongIndex = 0; alongIndex <= 40; ++alongIndex)
        {
            const auto z = -1.0F + (static_cast<float>(alongIndex) * 0.05F);
            const auto footing =
                antwika::collision::groundHeightUnderFootprint(
                    filledVoxels, x, z, 2.5F);

            if (!footing.has_value())
            {
                continue;
            }

            const auto secondHeight =
                antwika::collision::groundHeightUnderFootprint(
                    filledVoxels, x, z, *footing);

            ASSERT_TRUE(secondHeight.has_value());
            EXPECT_NEAR(*secondHeight, *footing, kTolerance);
        }
    }
}

TEST(WalkerTest, StoodCells_NamesTheCubeAWalkerIsInAndTheOneUnderIt)
{
    const auto supportCells = stoodCells(
        antwika::component::Position{.x = 2.9F, .y = 2.0F, .z = -3.1F});

    EXPECT_EQ(supportCells.front(), (VoxelPosition{.x = 2, .y = 2, .z = -4}));
    EXPECT_EQ(supportCells.back(), (VoxelPosition{.x = 2, .y = 1, .z = -4}));
}
