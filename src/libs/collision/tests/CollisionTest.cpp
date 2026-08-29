#include <gtest/gtest.h>

#include <cmath>

#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/voxel/VoxelStairs.hpp>
#include <antwika/collision/Collision.hpp>

using antwika::voxel::Voxels;
using antwika::collision::isSolid;
using antwika::collision::getSpawnPosition;
using antwika::collision::getRestPositionOverColumn;
using antwika::collision::getGroundHeightAtColumn;
using antwika::voxel::Kind;
using antwika::voxel::kVoxelSide;
using antwika::collision::kWalkSpeed;
using antwika::collision::positionOf;
using antwika::collision::getStoodCells;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::collision::hasHeadroom;
using antwika::component::Position;
using antwika::collision::positionFrom;
using antwika::collision::getMovedWithCollision;
using antwika::component::Velocity;
using antwika::voxel::VoxelPosition;

namespace
{

    constexpr float kTolerance = 0.0001F;

    constexpr float kTread =
        kVoxelSide
        / static_cast<float>(antwika::voxel::kStepsPerVoxel);

    [[nodiscard]] Voxels getFloorOver(
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

    [[nodiscard]] Voxels getFilledOver(
        const std::int32_t reach)
    {
        const auto voxels = getFloorOver(reach);

        return Voxels(voxels.begin(), voxels.end());
    }

}

TEST(CollisionTest, PositionFrom_TakesBackWhatPositionOfGave)
{
    const antwika::geometry::Vec3 position{3.25F, 6.5F, -2.125F};
    const auto stoodPosition = positionFrom(position);

    EXPECT_FLOAT_EQ(stoodPosition.x, position.x);
    EXPECT_FLOAT_EQ(stoodPosition.y, position.y);
    EXPECT_FLOAT_EQ(stoodPosition.z, position.z);
    EXPECT_FLOAT_EQ(positionOf(stoodPosition).x, position.x);
    EXPECT_FLOAT_EQ(positionOf(stoodPosition).y, position.y);
    EXPECT_FLOAT_EQ(positionOf(stoodPosition).z, position.z);
}

TEST(CollisionTest, IsSolid_TakesStoneAndStairsAndNotWaterOrAir)
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

TEST(CollisionTest, HasHeadroom_IsDeniedByWhateverStandsWithinAWalker)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 2, .z = 0}}});

    EXPECT_FALSE(hasHeadroom(filledVoxels, VoxelPosition{.x = 0, .y = 0,
        .z = 0}));
    EXPECT_TRUE(hasHeadroom(filledVoxels, VoxelPosition{.x = 0, .y = 2,
        .z = 0}));
}

TEST(CollisionTest, GroundHeightAtColumn_GivesTheTopOfWhatBearsUnderneath)
{
    const auto filledVoxels = getFilledOver(1);
    const auto footing = getGroundHeightAtColumn(filledVoxels, 0, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 1.0F, kTolerance);
}

TEST(CollisionTest, GroundHeightAtColumn_TakesAStepUpOfOneVoxel)
{
    auto filledVoxels = getFilledOver(1);
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));

    const auto footing = getGroundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 2.0F, kTolerance);
}

TEST(CollisionTest, GroundHeightAtColumn_TakesAStepDownOfOneVoxel)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = -1, .z = 0}}});

    const auto footing = getGroundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 0.0F, kTolerance);
}

TEST(CollisionTest, GroundHeightAtColumn_DropsToGroundFarBelow)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = -2, .z = 0}}});

    const auto footing = getGroundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, -1.0F, kTolerance);
    EXPECT_FALSE(getGroundHeightAtColumn(filledVoxels, 2, 0, 1.0F).has_value());
}

TEST(CollisionTest, GroundHeightAtColumn_GivesUpPastTheMaxFallDepth)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1,
            .y = -antwika::collision::kMaxFallDepth - 2, .z = 0}}});

    EXPECT_FALSE(getGroundHeightAtColumn(filledVoxels, 1, 0, 1.0F).has_value());
}

TEST(CollisionTest, GroundHeightAtColumn_StandsInWaterSunkToTheWaist)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}}});

    const auto footing = getGroundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 0.5F, kTolerance);
}

TEST(CollisionTest, MovedWithCollision_FallsNoFasterThanTheFallSpeed)
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

        stoodPosition = getMovedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F,
                .velocityZ = 0.0F});
        sankMost = std::max(sankMost, was.y - stoodPosition.y);
    }

    EXPECT_NEAR(stoodPosition.y, 1.0F, kTolerance);
    EXPECT_GT(stoodPosition.x, 2.0F);
    EXPECT_LE(
        sankMost, antwika::collision::kFallSpeed + kTolerance);
}

TEST(CollisionTest, MovedWithCollision_SinksWhereItStandsOverAHole)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = -3, .z = 0}}});

    auto stoodPosition = Position{.x = 0.5F, .y = 3.0F, .z = 0.5F};

    for (std::size_t step = 0; step < 60; ++step)
    {
        stoodPosition = getMovedWithCollision(filledVoxels, stoodPosition,
            Velocity{});
    }

    EXPECT_NEAR(stoodPosition.y, -2.0F, kTolerance);
}

TEST(CollisionTest, MovedWithCollision_WadesAtTheWadersPace)
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
        dryPosition = getMovedWithCollision(landVoxels, dryPosition,
        Velocity{.velocityX = 1.0F});
        wetPosition = getMovedWithCollision(poolVoxels, wetPosition,
            Velocity{.velocityX = 1.0F});
    }

    EXPECT_NEAR(
        (wetPosition.x + 1.5F) / (dryPosition.x + 1.5F),
        antwika::collision::kWaterSpeedFactor,
        0.01F);
}

TEST(CollisionTest, MovedWithCollision_RunsByTheMultiplierItIsSent)
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
            getMovedWithCollision(
                filledVoxels, walkedPosition, Velocity{.velocityX = 1.0F});
        ran = getMovedWithCollision(
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

TEST(CollisionTest, GroundHeightAtColumn_RefusesGroundWithNoRoomOverIt)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 1, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 2, .z = 0}}});

    EXPECT_FALSE(getGroundHeightAtColumn(filledVoxels, 1, 0, 1.0F).has_value());
}

TEST(CollisionTest, RestPositionOverColumn_TakesTheHighestOfAColumnWithRoom)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 1, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 4, .z = 0}}});

    const auto foothold = getRestPositionOverColumn(filledVoxels, 0, 0);

    ASSERT_TRUE(foothold.has_value());
    EXPECT_NEAR(foothold->y, 5.0F, kTolerance);
}

TEST(CollisionTest, RestPositionOverColumn_GivesNothingOverAnEmptyColumn)
{
    EXPECT_FALSE(getRestPositionOverColumn(getFilledOver(1), 8, 8).has_value());
}

TEST(CollisionTest, SpawnPosition_GivesNothingOnAPileThatBearsNothing)
{
    EXPECT_FALSE(getSpawnPosition(Voxels{}).has_value());
}

TEST(CollisionTest, SpawnPosition_StandsOnThePile)
{
    const auto foothold = getSpawnPosition(getFloorOver(2));

    ASSERT_TRUE(foothold.has_value());
    EXPECT_NEAR(foothold->y, 1.0F, kTolerance);
    EXPECT_NEAR(foothold->x, 0.0F, kTolerance);
    EXPECT_NEAR(foothold->z, 0.0F, kTolerance);
}

TEST(CollisionTest, MovedWithCollision_CarriesAWalkerAcrossTheGround)
{
    const auto filledVoxels = getFilledOver(2);
    const auto stoodPosition = getMovedWithCollision(
        filledVoxels,
        Position{.x = 0.5F, .y = 1.0F, .z = 0.5F},
        Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});

    EXPECT_NEAR(stoodPosition.x, 0.5F + kWalkSpeed, kTolerance);
    EXPECT_NEAR(stoodPosition.y, 1.0F, kTolerance);
    EXPECT_NEAR(stoodPosition.z, 0.5F, kTolerance);
}

TEST(CollisionTest, MovedWithCollision_CrossesARampAtHalfItsPace)
{
    auto filledVoxels = getFilledOver(2);

    filledVoxels.erase(VoxelPosition{.x = 0, .y = 0, .z = 0});
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
        .material = {.kind = Kind::Ramp}}}));

    const auto stoodPosition = getMovedWithCollision(
        filledVoxels,
        Position{.x = 0.5F, .y = 1.0F, .z = 0.5F},
        Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});

    EXPECT_NEAR(
        stoodPosition.x,
        0.5F + (kWalkSpeed * antwika::collision::kRampSpeedFactor),
        kTolerance);
}

TEST(CollisionTest, MovedWithCollision_LeavesAWalkerSentNowhereWhereItStood)
{
    const auto filledVoxels = getFilledOver(2);
    const Position stoodPosition{.x = 0.75F, .y = 1.0F, .z = 0.25F};
    const auto walkedTo = getMovedWithCollision(filledVoxels, stoodPosition,
        Velocity{});

    EXPECT_NEAR(walkedTo.x, stoodPosition.x, kTolerance);
    EXPECT_NEAR(walkedTo.y, stoodPosition.y, kTolerance);
    EXPECT_NEAR(walkedTo.z, stoodPosition.z, kTolerance);
}

TEST(CollisionTest, MovedWithCollision_ClimbsAStepOfOneVoxel)
{
    auto filledVoxels = getFilledOver(2);
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));

    const auto stoodPosition = getMovedWithCollision(
        filledVoxels,
        Position{.x = 0.95F, .y = 1.0F, .z = 0.5F},
        Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});

    EXPECT_NEAR(stoodPosition.y, 2.0F, kTolerance);
}

TEST(CollisionTest, MovedWithCollision_IsStoppedByASideTallerThanAStep)
{
    auto filledVoxels = getFilledOver(2);
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 2,
        .z = 0}}}));

    const Position stoodPosition{.x = 0.95F, .y = 1.0F, .z = 0.5F};
    const auto walkedTo =
        getMovedWithCollision(
            filledVoxels,
            stoodPosition,
            Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});

    EXPECT_NEAR(walkedTo.x, stoodPosition.x, kTolerance);
    EXPECT_NEAR(walkedTo.y, stoodPosition.y, kTolerance);
}

TEST(
    CollisionTest,
    MovedWithCollision_RunsAlongWhatHoldsItRatherThanStoppingDead)
{
    auto filledVoxels = getFilledOver(4);

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
        stoodPosition = getMovedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F,
                .velocityZ = 1.0F});
    }

    EXPECT_LT(
        stoodPosition.x,
        1.0F - (antwika::collision::kFootprintWidth / 2.0F) + kTolerance);
    EXPECT_GT(stoodPosition.z, 0.5F);
}

TEST(CollisionTest, MovedWithCollision_KeepsOneWayAboutFromOutrunningTwo)
{
    const auto filledVoxels = getFilledOver(4);
    const Position stoodPosition{.x = 0.5F, .y = 1.0F, .z = 0.5F};
    const auto walkedTo =
        getMovedWithCollision(
            filledVoxels,
            stoodPosition,
            Velocity{.velocityX = 1.0F, .velocityZ = 1.0F});
    const auto wentX = walkedTo.x - stoodPosition.x;
    const auto wentZ = walkedTo.z - stoodPosition.z;
    const auto went = std::sqrt((wentX * wentX) + (wentZ * wentZ));

    EXPECT_NEAR(went, kWalkSpeed, kTolerance);
}

TEST(
    CollisionTest,
    MovedWithCollision_SettlesAWalkerLeftOverGroundThatFellAway)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = -1, .z = 0}}});
    auto stoodPosition = Position{.x = 0.5F, .y = 1.0F, .z = 0.5F};

    for (std::size_t step = 0; step < 10; ++step)
    {
        stoodPosition = getMovedWithCollision(filledVoxels, stoodPosition,
            Velocity{});
    }

    EXPECT_NEAR(stoodPosition.y, 0.0F, kTolerance);
}

TEST(CollisionTest, MovedWithCollision_ClimbsAFlightLaidAsARampCube)
{
    const auto filledVoxels = antwika::voxel::withBlockAt(
        Voxels{},
        VoxelPosition{.x = 0, .y = 0, .z = 0},
        Kind::Ramp,
        antwika::voxel::Facing::East);

    const auto lowGround = getGroundHeightAtColumn(filledVoxels, 0, 0, 1.0F);
    const auto highGround = getGroundHeightAtColumn(filledVoxels, 1, 0, 1.0F);

    ASSERT_TRUE(lowGround.has_value());
    ASSERT_TRUE(highGround.has_value());
    EXPECT_NEAR(*lowGround, 1.0F, kTolerance);
    EXPECT_NEAR(*highGround, 2.0F, kTolerance);

    auto stoodPosition = Position{.x = 0.5F, .y = *lowGround, .z = 0.5F};

    for (auto step = 0; step < 20; ++step)
    {
        stoodPosition = getMovedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F,
                .velocityZ = 0.0F});
    }

    EXPECT_GT(stoodPosition.x, 0.5F);
    EXPECT_GT(stoodPosition.y, *lowGround);
    EXPECT_LE(stoodPosition.y, *highGround);

    const auto underCell =
        antwika::collision::getSupportingVoxel(filledVoxels, 1, 0,
            stoodPosition.y);

    ASSERT_TRUE(underCell.has_value());
    EXPECT_NEAR(
        stoodPosition.y,
        antwika::collision::getGroundHeightOn(filledVoxels, *underCell,
        stoodPosition.x,
            stoodPosition.z),
        kTolerance);
}

TEST(
    CollisionTest,
    GroundHeightUnderFootprint_HoldsEveryColumnItsFootprintCovers)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 1}}});

    EXPECT_TRUE(
        antwika::collision::getGroundHeightUnderFootprint(
            filledVoxels,
            0.5F,
            0.5F,
            1.0F)
            .has_value());
    EXPECT_FALSE(
        antwika::collision::getGroundHeightUnderFootprint(
            filledVoxels,
            2.0F,
            0.5F,
            1.0F)
            .has_value());
}

TEST(
    CollisionTest,
    GroundHeightUnderFootprint_RestsOnTheHighestUnderItsFootprint)
{
    auto filledVoxels = getFilledOver(2);
    filledVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 1, .y = 1,
        .z = 0}}}));

    const auto footing =
        antwika::collision::getGroundHeightUnderFootprint(
            filledVoxels, 1.1F, 0.5F, 1.0F);

    ASSERT_TRUE(footing.has_value());
    EXPECT_NEAR(*footing, 2.0F, kTolerance);
}

TEST(
    CollisionTest,
    MovedWithCollision_KeepsAWalkersFootprintOffTheEdgeOfADrop)
{
    const auto filledVoxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = -1}}});

    auto stoodPosition = Position{.x = 0.5F, .y = 1.0F, .z = 0.5F};

    for (auto step = 0; step < 20; ++step)
    {
        stoodPosition = getMovedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F,
                .velocityZ = 0.0F});
    }

    EXPECT_GT(stoodPosition.x, 0.0F);
    EXPECT_LT(
        stoodPosition.x,
        1.0F - (antwika::collision::kFootprintWidth / 2.0F) + kTolerance);
}

TEST(CollisionTest, KWalkerPixel_MeasuresTheFootprintInWholePixels)
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

    [[nodiscard]] Voxels getRampEastward()
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

    [[nodiscard]] Voxels getWalledRamp()
    {
        auto filledVoxels = getRampEastward();

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

TEST(CollisionTest, GroundHeightOn_StandsAPlainVoxelAtItsTopThroughout)
{
    const auto filledVoxels = getFilledOver(2);
    const VoxelCell groundCell{.position = {.x = 0, .y = 0, .z = 0}};

    for (const auto index : {0.1F, 0.5F, 0.9F})
    {
        EXPECT_NEAR(
            antwika::collision::getGroundHeightOn(filledVoxels, groundCell, index,
            index),
            1.0F * kVoxelSide,
            kTolerance);
    }
}

TEST(CollisionTest, GroundHeightOn_RisesEvenlyAcrossARamp)
{
    const auto filledVoxels = getRampEastward();
    const VoxelCell rampCell{
        .position = {.x = 1, .y = 1, .z = 0},
        .material = {
            .kind = Kind::Ramp,
            .facing = antwika::voxel::Facing::East}};

    EXPECT_NEAR(
        antwika::collision::getGroundHeightOn(filledVoxels, rampCell, 1.0F, 0.5F),
        1.0F,
        kTolerance);
    EXPECT_NEAR(
        antwika::collision::getGroundHeightOn(filledVoxels, rampCell, 1.5F, 0.5F),
        1.5F,
        kTolerance);
    EXPECT_NEAR(
        antwika::collision::getGroundHeightOn(filledVoxels, rampCell, 2.0F, 0.5F),
        2.0F,
        kTolerance);
}

TEST(CollisionTest, GroundHeightOn_HoldsToTheEndsOfARampBeyondIt)
{
    const auto filledVoxels = getRampEastward();
    const VoxelCell rampCell{
        .position = {.x = 1, .y = 1, .z = 0},
        .material = {
            .kind = Kind::Ramp,
            .facing = antwika::voxel::Facing::East}};

    EXPECT_NEAR(
        antwika::collision::getGroundHeightOn(filledVoxels, rampCell, -1.5F, 0.5F),
        1.0F,
        kTolerance);
    EXPECT_NEAR(
        antwika::collision::getGroundHeightOn(filledVoxels, rampCell, 4.5F, 0.5F),
        2.0F,
        kTolerance);
}

TEST(CollisionTest, MovedWithCollision_LiftsAWalkerByATreadAndNoMoreAtOnce)
{
    const auto filledVoxels = getRampEastward();
    Position stoodPosition{.x = -0.5F, .y = 1.0F, .z = 0.5F};
    auto liftedHeight = 0.0F;
    auto didClimb = false;

    for (std::size_t step = 0; step < 60; ++step)
    {
        const auto was = stoodPosition;

        stoodPosition = getMovedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = 1.0F});
        liftedHeight = std::max(liftedHeight,
            std::abs(stoodPosition.y - was.y));
        didClimb = didClimb || stoodPosition.y > was.y;
    }

    EXPECT_TRUE(didClimb);
    EXPECT_NEAR(stoodPosition.y, 2.0F, kTolerance);
    EXPECT_LE(liftedHeight, kTread + kWalkSpeed + kTolerance);
}

TEST(CollisionTest, MovedWithCollision_DropsAWalkerByATreadAndNoMoreAtOnce)
{
    const auto filledVoxels = getRampEastward();
    Position stoodPosition{.x = 3.5F, .y = 2.0F, .z = 0.5F};
    auto droppedHeight = 0.0F;

    for (std::size_t step = 0; step < 60; ++step)
    {
        const auto was = stoodPosition;

        stoodPosition = getMovedWithCollision(
            filledVoxels, stoodPosition, Velocity{.velocityX = -1.0F});
        droppedHeight = std::max(droppedHeight,
            std::abs(stoodPosition.y - was.y));
    }

    EXPECT_NEAR(stoodPosition.y, 1.0F, kTolerance);
    EXPECT_LE(droppedHeight, kTread + kWalkSpeed + kTolerance);
}

TEST(
    CollisionTest,
    GroundHeightUnderFootprint_StandsWhereEveryColumnBearsItStill)
{
    const auto filledVoxels = getWalledRamp();

    for (auto index = 0; index <= 60; ++index)
    {
        const auto x = -1.0F + (static_cast<float>(index) * 0.05F);

        for (auto alongIndex = 0; alongIndex <= 40; ++alongIndex)
        {
            const auto z = -1.0F + (static_cast<float>(alongIndex) * 0.05F);
            const auto footing =
                antwika::collision::getGroundHeightUnderFootprint(
                    filledVoxels, x, z, 2.5F);

            if (!footing.has_value())
            {
                continue;
            }

            const auto secondHeight =
                antwika::collision::getGroundHeightUnderFootprint(
                    filledVoxels, x, z, *footing);

            ASSERT_TRUE(secondHeight.has_value());
            EXPECT_NEAR(*secondHeight, *footing, kTolerance);
        }
    }
}

TEST(CollisionTest, StoodCells_NamesTheCubeAWalkerIsInAndTheOneUnderIt)
{
    const auto supportCells = getStoodCells(
        antwika::component::Position{.x = 2.9F, .y = 2.0F, .z = -3.1F});

    EXPECT_EQ(supportCells.front(), (VoxelPosition{.x = 2, .y = 2, .z = -4}));
    EXPECT_EQ(supportCells.back(), (VoxelPosition{.x = 2, .y = 1, .z = -4}));
}

namespace
{

    [[nodiscard]] Voxels getRampBesideAFloor()
    {
        auto filledVoxels = antwika::voxel::withBlockAt(
            Voxels{},
            VoxelPosition{.x = 0, .y = 0, .z = 0},
            Kind::Ramp,
            antwika::voxel::Facing::East);

        return antwika::voxel::withBlockAt(
            filledVoxels, VoxelPosition{.x = 0, .y = 0, .z = 2});
    }

    [[nodiscard]] Position getWalkedAlong(
        const Voxels &filledVoxels, Position stoodPosition)
    {
        for (auto step = 0; step < 20; ++step)
        {
            stoodPosition = getMovedWithCollision(
                filledVoxels,
                stoodPosition,
                Velocity{.velocityX = 0.0F, .velocityZ = 1.0F});
        }

        return stoodPosition;
    }

}

TEST(
    CollisionTest,
    MovedWithCollision_KeepsTheFloorBesideTheFootOfARampOutOfReach)
{
    const auto filledVoxels = getRampBesideAFloor();
    const auto walkedTo = getWalkedAlong(
        filledVoxels, Position{.x = 0.5F, .y = 0.5F, .z = 1.5F});

    EXPECT_LT(walkedTo.z, 2.0F);
    EXPECT_LT(walkedTo.y, 1.0F);
}

TEST(
    CollisionTest,
    MovedWithCollision_KeepsTheFloorBesideTheMiddleOfARampOutOfReach)
{
    const auto filledVoxels = getRampBesideAFloor();
    const auto walkedTo = getWalkedAlong(
        filledVoxels, Position{.x = 1.0F, .y = 1.0F, .z = 1.5F});

    EXPECT_LT(walkedTo.z, 2.0F);
    EXPECT_NEAR(walkedTo.y, 1.0F, kTolerance);
}

TEST(
    CollisionTest,
    MovedWithCollision_TakesTheFloorBesideTheTopOfARamp)
{
    const auto filledVoxels = getRampBesideAFloor();
    const auto walkedTo = getWalkedAlong(
        filledVoxels, Position{.x = 1.5F, .y = 1.5F, .z = 1.5F});

    EXPECT_GT(walkedTo.z, 2.0F);
    EXPECT_NEAR(walkedTo.y, 2.0F, kTolerance);
}
