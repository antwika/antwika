#include <gtest/gtest.h>
#include <glm/geometric.hpp>

#include <cmath>
#include <cstddef>
#include <iterator>
#include <set>
#include <vector>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelDetail.hpp>
#include <antwika/voxel/VoxelStairs.hpp>

using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;
using antwika::voxel::VoxelMaterial;
using antwika::voxel::voxelsOf;
using antwika::voxel::Voxels;

TEST(VoxelStairsTest, InferredRampDirection_RisesTowardsWhatStandsBesideIt)
{
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1},
            .material = {.kind = Kind::Normal}}});
    const auto climb = getInferredRampDirection(voxels, VoxelPosition{});

    EXPECT_EQ(climb.x, 0);
    EXPECT_EQ(climb.y, 0);
    EXPECT_EQ(climb.z, 1);
}



TEST(
    VoxelStairsTest,
    InferredRampDirection_RisesAwayFromTheWedgeUnderItsOpenSide)
{
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 1, .z = 0},
            .material = {.kind = Kind::Ramp}}});
    const auto climb = getInferredRampDirection(
        voxels, VoxelPosition{.x = 1, .y = 1, .z = 0});

    EXPECT_EQ(climb.x, 1);
    EXPECT_EQ(climb.z, 0);
}



TEST(VoxelStairsTest, StairQuads_BuildsAFlightOfStepsWithinTheVoxel)
{
    using antwika::voxel::kStairQuads;
    using antwika::voxel::getStairQuads;

    const auto flight = getStairQuads(VoxelPosition{.x = 1});

    EXPECT_EQ(flight.size(), kStairQuads);

    for (const auto &quad : flight)
    {
        for (const auto corner : quad.corners)
        {
            EXPECT_LE(std::abs(corner.x), 0.5F + 1e-4F);
            EXPECT_LE(std::abs(corner.y), 0.5F + 1e-4F);
            EXPECT_LE(std::abs(corner.z), 0.5F + 1e-4F);
        }
    }
}



TEST(VoxelStairsTest, StairQuads_KeepsEveryStepSquareOn)
{
    using antwika::voxel::getStairQuads;

    for (const auto climb :
         {VoxelPosition{.x = 1}, VoxelPosition{.x = -1},
          VoxelPosition{.z = 1}, VoxelPosition{.z = -1}})
    {
        for (const auto &quad : getStairQuads(climb))
        {
            const auto acrossVector = quad.corners[1] - quad.corners[0];
            const auto downVector = quad.corners[3] - quad.corners[0];
            const auto normal = glm::normalize(
                glm::cross(acrossVector, downVector));
            const auto sideNormal =
                antwika::voxel::detail::kVoxelFaces.at(quad.side).normal;

            EXPECT_NEAR(
                glm::dot(normal, sideNormal), 1.0F, 1e-4F);
        }
    }
}



TEST(VoxelStairsTest, StairQuads_LaysTheLowestTreadFlatInTheFloor)
{
    using antwika::voxel::kStepsPerVoxel;
    using antwika::voxel::getStairQuads;

    const auto share =
        1.0F / static_cast<float>(kStepsPerVoxel);

    std::set<float> tops;

    for (const auto &quad : getStairQuads(VoxelPosition{.z = -1}))
    {
        if (antwika::voxel::detail::kVoxelFaces.at(quad.side).normal.y > 0.5F)
        {
            tops.insert(quad.corners[0].y);
        }
    }

    EXPECT_EQ(tops.size(), kStepsPerVoxel);
    EXPECT_NEAR(*tops.begin(), -0.5F, 1e-4F);
    EXPECT_NEAR(*tops.rbegin(), 0.5F - share, 1e-4F);
}



TEST(VoxelStairsTest, StairQuads_StandsTheLastRiserUpToTheVoxelsTop)
{
    using antwika::voxel::getStairQuads;

    auto tallest = -1.0F;

    for (const auto &quad : getStairQuads(VoxelPosition{.z = -1}))
    {
        for (const auto corner : quad.corners)
        {
            tallest = std::max(tallest, corner.y);
        }
    }

    EXPECT_NEAR(tallest, 0.5F, 1e-4F);
}



TEST(VoxelStairsTest, StairQuads_TurnsTheFlightWithItsClimb)
{
    using antwika::voxel::getStairQuads;

    const auto east = getStairQuads(VoxelPosition{.x = 1});
    const auto north = getStairQuads(VoxelPosition{.z = -1});

    EXPECT_NE(east, north);
    EXPECT_EQ(east.size(), north.size());
}



TEST(VoxelStairsTest, InferredRampDirection_TurnsARampCubeTheWayItWasLaid)
{
    using antwika::voxel::getCubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;

    for (const auto climb :
         {VoxelPosition{.x = 1}, VoxelPosition{.x = -1},
          VoxelPosition{.z = 1}, VoxelPosition{.z = -1}})
    {
        Voxels voxels;

        for (std::int32_t x = -4; x <= 6; ++x)
        {
            for (std::int32_t z = -4; z <= 6; ++z)
            {
                voxels[VoxelPosition{.x = x, .y = -1, .z = z}] =
                    VoxelMaterial{};
            }
        }

        for (const auto &[position, material] :
             getCubeVoxels(VoxelPosition{}, Kind::Ramp, climb))
        {
            voxels[position] = material;
        }

        for (const auto &[position, material] : voxels)
        {
            const auto aboveCell = VoxelPosition{
                .x = position.x, .y = position.y + 1, .z = position.z};

            if (material.kind != Kind::Ramp || voxels.contains(aboveCell))
            {
                continue;
            }

            const auto direction =
                getInferredRampDirection(voxels, position);

            EXPECT_EQ(direction.x, climb.x);
            EXPECT_EQ(direction.z, climb.z);
        }
    }
}



TEST(VoxelStairsTest, InferredRampDirection_RisesTowardsACubeBesideIt)
{
    using antwika::voxel::getCubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::getInferredRampDirection;

    const VoxelPosition climbPosition{.z = 1};

    Voxels voxels;

    for (std::int32_t x = -4; x <= 6; ++x)
    {
        for (std::int32_t z = -4; z <= 6; ++z)
        {
            voxels[VoxelPosition{.x = x, .y = -1, .z = z}] = VoxelMaterial{};
        }
    }

    for (const auto &[position, material] :
         getCubeVoxels(VoxelPosition{}, Kind::Ramp, climbPosition))
    {
        voxels[position] = material;
    }

    for (const auto &[position, material] :
         getCubeVoxels( VoxelPosition{.z = kCubeSide}, Kind::Normal,
             climbPosition))
    {
        voxels[position] = material;
    }

        for (const auto &[position, material] : voxels)
    {
        const auto aboveCell =
            VoxelPosition{.x = position.x, .y = position.y + 1,
                .z = position.z};

        if (material.kind != Kind::Ramp || voxels.contains(aboveCell))
        {
            continue;
        }

        EXPECT_EQ(getInferredRampDirection(voxels, position).z, 1);
        EXPECT_EQ(getInferredRampDirection(voxels, position).x, 0);
    }
}



TEST(VoxelStairsTest, FacingOfStep_NamesTheWayAStepClimbs)
{
    using antwika::voxel::Facing;
    using antwika::voxel::getFacingOfStep;

    EXPECT_EQ(getFacingOfStep(VoxelPosition{.x = 1}), Facing::East);
    EXPECT_EQ(getFacingOfStep(VoxelPosition{.x = -1}), Facing::West);
    EXPECT_EQ(getFacingOfStep(VoxelPosition{.z = 1}), Facing::South);
    EXPECT_EQ(getFacingOfStep(VoxelPosition{.z = -1}), Facing::North);
    EXPECT_EQ(getFacingOfStep(VoxelPosition{}), Facing::Any);
}



TEST(VoxelStairsTest, InferredRampDirection_TakesTheWayTheRampAboveItClimbs)
{
    using antwika::voxel::getCubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;

    const VoxelPosition climbPosition{.z = 1};

    Voxels voxels;

    for (std::int32_t x = -4; x <= 6; ++x)
    {
        for (std::int32_t z = -4; z <= 6; ++z)
        {
            voxels[VoxelPosition{.x = x, .y = -1, .z = z}] = VoxelMaterial{};
        }
    }

    for (const auto &[position, material] :
         getCubeVoxels(VoxelPosition{}, Kind::Ramp, climbPosition))
    {
        voxels[position] = material;
    }

        for (const auto &[position, material] : voxels)
    {
        const auto aboveCell =
            VoxelPosition{.x = position.x, .y = position.y + 1,
                .z = position.z};

        if (material.kind != Kind::Ramp || !voxels.contains(aboveCell))
        {
            continue;
        }

        EXPECT_EQ(getInferredRampDirection(voxels, position).z, climbPosition.z);
        EXPECT_EQ(getInferredRampDirection(voxels, position).x, climbPosition.x);
    }
}




TEST(VoxelStairsTest, StairHalfOf_StandsAFlightOnTwoLevels)
{
    using antwika::voxel::getCubeVoxels;
    using antwika::voxel::stairHalfOf;
    using antwika::voxel::Kind;
    using antwika::voxel::StairHalf;

    Voxels voxels;

    for (const auto &[position, material] :
         getCubeVoxels(VoxelPosition{}, Kind::Ramp, VoxelPosition{.x = 1}))
    {
        voxels[position] = material;
    }

    std::map<StairHalf, std::size_t> countByHalf;

    for (const auto &[position, material] : voxels)
    {
        countByHalf[stairHalfOf(voxels, position)] += 1;
    }

    EXPECT_GT(countByHalf[StairHalf::Lower], 0U);
    EXPECT_GT(countByHalf[StairHalf::Upper], 0U);
    EXPECT_EQ(countByHalf[StairHalf::Any], 0U);
}



TEST(VoxelStairsTest, StairHalfOf_StandsAStepOnAnotherAtTheUpperLevel)
{
    using antwika::voxel::stairHalfOf;
    using antwika::voxel::Kind;
    using antwika::voxel::StairHalf;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 0, .y = 1, .z = 0},
            .material = {.kind = Kind::Ramp}}});

    EXPECT_EQ(stairHalfOf(voxels, VoxelPosition{}), StairHalf::Lower);
    EXPECT_EQ(
        stairHalfOf(voxels, VoxelPosition{.y = 1}), StairHalf::Upper);
}



TEST(VoxelStairsTest, StairHalfOf_StandsAVoxelThatIsNoStepAtNoLevel)
{
    using antwika::voxel::stairHalfOf;
    using antwika::voxel::StairHalf;

    const auto voxels = voxelsOf({VoxelCell{}});

    EXPECT_EQ(stairHalfOf(voxels, VoxelPosition{}), StairHalf::Any);
}



TEST(VoxelStairsTest, InferredRampDirection_TakesTheWayAVoxelWasTold)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;
    using antwika::voxel::stepVectorFor;

    for (const auto told :
         {Facing::East, Facing::West, Facing::North, Facing::South})
    {
        const auto voxels = voxelsOf({
            VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
                .material = {.kind = Kind::Ramp, .facing = told}},
            VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
                .material = {.kind = Kind::Normal}}});

        EXPECT_EQ(
            getInferredRampDirection(voxels, VoxelPosition{}),
            stepVectorFor(told));
    }
}



TEST(VoxelStairsTest, InferredRampDirection_ReckonsAVoxelToldNothing)
{
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}}});

    EXPECT_EQ(
        getInferredRampDirection(voxels, VoxelPosition{}),
        VoxelPosition{.x = 1});
}



TEST(VoxelStairsTest, InferredRampDirection_HoldsAToldWayWithNothingBesideIt)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;
    using antwika::voxel::stepVectorFor;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp, .facing = Facing::North}}});

    EXPECT_EQ(
        getInferredRampDirection(voxels, VoxelPosition{}),
        stepVectorFor(Facing::North));
}



TEST(VoxelStairsTest, StepVectorFor_NamesTheStepEachFacingStandsFor)
{
    using antwika::voxel::Facing;
    using antwika::voxel::getFacingOfStep;
    using antwika::voxel::stepVectorFor;

    for (const auto facing :
         {Facing::East, Facing::West, Facing::North, Facing::South})
    {
        EXPECT_EQ(getFacingOfStep(stepVectorFor(facing)), facing);
    }

    EXPECT_EQ(stepVectorFor(Facing::Any), VoxelPosition{});
}



TEST(VoxelStairsTest, InferredRampDirection_KeepsAWholeCubeOfOneMind)
{
    using antwika::voxel::getCubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;

    for (const auto climbPosition :
         {VoxelPosition{.x = 1},
          VoxelPosition{.x = -1},
          VoxelPosition{.z = 1},
          VoxelPosition{.z = -1}})
    {
        Voxels voxels;

        for (const auto &[position, material] :
             getCubeVoxels(VoxelPosition{}, Kind::Ramp, climbPosition))
        {
            voxels[position] = material;
        }

        for (const auto &[position, material] :
             getCubeVoxels( VoxelPosition{.x = 2}, Kind::Ramp,
                 VoxelPosition{.x = -1}))
        {
            voxels[position] = material;
        }

        for (const auto &[position, material] : voxels)
        {
            if (material.kind != Kind::Ramp || position.x > 1)
            {
                continue;
            }

            EXPECT_EQ(getInferredRampDirection(voxels, position), climbPosition);
        }
    }
}



TEST(
    VoxelStairsTest,
    InferredRampDirection_LeavesARampBesideAnotherToItsOwnShape)
{
    using antwika::voxel::getCubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::getInferredRampDirection;

    Voxels voxels;

    for (const auto &[position, material] :
         getCubeVoxels(VoxelPosition{}, Kind::Ramp, VoxelPosition{.z = -1}))
    {
        voxels[position] = material;
    }

    for (const auto &[position, material] :
         getCubeVoxels( VoxelPosition{.x = 2}, Kind::Ramp, VoxelPosition{.z = -1}))
    {
        voxels[position] = material;
    }

    for (const auto &[position, material] : voxels)
    {
        EXPECT_EQ(
            getInferredRampDirection(voxels, position), VoxelPosition{.z = -1});
    }
}
