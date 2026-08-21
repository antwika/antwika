#include <gtest/gtest.h>

#include <glm/geometric.hpp>

#include <cmath>
#include <cstddef>
#include <set>
#include <vector>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelDetail.hpp>
#include <antwika/voxel/VoxelStairs.hpp>

using antwika::voxel::VoxelCell;

TEST(VoxelStairsTest, InferredRampDirection_RisesTowardsWhatStandsBesideIt)
{
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;

    const std::vector<VoxelCell> cells{
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 0, .y = 0, .z = 1, .kind = Kind::Normal}};
    const auto climb = inferredRampDirection(cells, cells.front());

    EXPECT_EQ(climb.x, 0);
    EXPECT_EQ(climb.y, 0);
    EXPECT_EQ(climb.z, 1);
}



TEST(
    VoxelStairsTest,
    InferredRampDirection_RisesAwayFromTheWedgeUnderItsOpenSide)
{
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;

    const std::vector<VoxelCell> cells{
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 1, .y = 0, .z = 0, .kind = Kind::Normal},
        VoxelCell{.x = 1, .y = 1, .z = 0, .kind = Kind::Ramp}};
    const auto climb = inferredRampDirection(cells, cells.back());

    EXPECT_EQ(climb.x, 1);
    EXPECT_EQ(climb.z, 0);
}



TEST(VoxelStairsTest, StairQuads_BuildsAFlightOfStepsWithinTheVoxel)
{
    using antwika::voxel::kStairQuads;
    using antwika::voxel::stairQuads;

    const auto flight = stairQuads(VoxelCell{.x = 1});

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
    using antwika::voxel::stairQuads;

    for (const auto climb :
         {VoxelCell{.x = 1}, VoxelCell{.x = -1},
          VoxelCell{.z = 1}, VoxelCell{.z = -1}})
    {
        for (const auto &quad : stairQuads(climb))
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
    using antwika::voxel::stairQuads;

    const auto share =
        1.0F / static_cast<float>(kStepsPerVoxel);

    std::set<float> tops;

    for (const auto &quad : stairQuads(VoxelCell{.z = -1}))
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
    using antwika::voxel::stairQuads;

    auto tallest = -1.0F;

    for (const auto &quad : stairQuads(VoxelCell{.z = -1}))
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
    using antwika::voxel::stairQuads;

    const auto east = stairQuads(VoxelCell{.x = 1});
    const auto north = stairQuads(VoxelCell{.z = -1});

    EXPECT_NE(east, north);
    EXPECT_EQ(east.size(), north.size());
}



TEST(VoxelStairsTest, InferredRampDirection_TurnsARampCubeTheWayItWasLaid)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;

    for (const auto climb :
         {VoxelCell{.x = 1}, VoxelCell{.x = -1},
          VoxelCell{.z = 1}, VoxelCell{.z = -1}})
    {
        std::vector<VoxelCell> cells;

        for (std::int32_t x = -4; x <= 6; ++x)
        {
            for (std::int32_t z = -4; z <= 6; ++z)
            {
                cells.push_back(
                    VoxelCell{.x = x, .y = -1, .z = z});
            }
        }

        for (const auto cell : cubeVoxels(VoxelCell{}, Kind::Ramp,
                                          climb))
        {
            cells.push_back(cell);
        }

        const std::set<VoxelCell> standingCells(
            cells.begin(), cells.end());

        for (const auto cell : cells)
        {
            const auto aboveCell = VoxelCell{
                .x = cell.x, .y = cell.y + 1, .z = cell.z};

            if (cell.kind != Kind::Ramp || standingCells.contains(aboveCell))
            {
                continue;
            }

            const auto direction =
                inferredRampDirection(cells, cell);

            EXPECT_EQ(direction.x, climb.x);
            EXPECT_EQ(direction.z, climb.z);
        }
    }
}



TEST(VoxelStairsTest, InferredRampDirection_RisesTowardsACubeBesideIt)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::inferredRampDirection;

    const VoxelCell climbCell{.z = 1};

    std::vector<VoxelCell> cells;

    for (std::int32_t x = -4; x <= 6; ++x)
    {
        for (std::int32_t z = -4; z <= 6; ++z)
        {
            cells.push_back(VoxelCell{.x = x, .y = -1, .z = z});
        }
    }

    for (const auto cell :
         cubeVoxels(VoxelCell{}, Kind::Ramp, climbCell))
    {
        cells.push_back(cell);
    }

    for (const auto cell : cubeVoxels(
             VoxelCell{.z = kCubeSide}, Kind::Normal, climbCell))
    {
        cells.push_back(cell);
    }

    const std::set<VoxelCell> standingCells(cells.begin(), cells.end());

    for (const auto cell : cells)
    {
        const auto aboveCell =
            VoxelCell{.x = cell.x, .y = cell.y + 1, .z = cell.z};

        if (cell.kind != Kind::Ramp || standingCells.contains(aboveCell))
        {
            continue;
        }

        EXPECT_EQ(inferredRampDirection(cells, cell).z, 1);
        EXPECT_EQ(inferredRampDirection(cells, cell).x, 0);
    }
}



TEST(VoxelStairsTest, FacingOfStep_NamesTheWayAStepClimbs)
{
    using antwika::voxel::Facing;
    using antwika::voxel::facingOfStep;

    EXPECT_EQ(facingOfStep(VoxelCell{.x = 1}), Facing::East);
    EXPECT_EQ(facingOfStep(VoxelCell{.x = -1}), Facing::West);
    EXPECT_EQ(facingOfStep(VoxelCell{.z = 1}), Facing::South);
    EXPECT_EQ(facingOfStep(VoxelCell{.z = -1}), Facing::North);
    EXPECT_EQ(facingOfStep(VoxelCell{}), Facing::Any);
}



TEST(VoxelStairsTest, InferredRampDirection_TakesTheWayTheRampAboveItClimbs)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;

    const VoxelCell climbCell{.z = 1};

    std::vector<VoxelCell> cells;

    for (std::int32_t x = -4; x <= 6; ++x)
    {
        for (std::int32_t z = -4; z <= 6; ++z)
        {
            cells.push_back(VoxelCell{.x = x, .y = -1, .z = z});
        }
    }

    for (const auto cell :
         cubeVoxels(VoxelCell{}, Kind::Ramp, climbCell))
    {
        cells.push_back(cell);
    }

    const std::set<VoxelCell> standingCells(cells.begin(), cells.end());

    for (const auto cell : cells)
    {
        const auto aboveCell =
            VoxelCell{.x = cell.x, .y = cell.y + 1, .z = cell.z};

        if (cell.kind != Kind::Ramp || !standingCells.contains(aboveCell))
        {
            continue;
        }

        EXPECT_EQ(inferredRampDirection(cells, cell).z, climbCell.z);
        EXPECT_EQ(inferredRampDirection(cells, cell).x, climbCell.x);
    }
}




TEST(VoxelStairsTest, StairHalfOf_StandsAFlightOnTwoLevels)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::stairHalfOf;
    using antwika::voxel::Kind;
    using antwika::voxel::StairHalf;

    std::vector<VoxelCell> cells;

    for (const auto cell :
         cubeVoxels(VoxelCell{}, Kind::Ramp, VoxelCell{.x = 1}))
    {
        cells.push_back(cell);
    }

    std::map<StairHalf, std::size_t> countByHalf;

    for (const auto cell : cells)
    {
        countByHalf[stairHalfOf(cells, cell)] += 1;
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

    const std::vector<VoxelCell> cells{
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 0, .y = 1, .z = 0, .kind = Kind::Ramp}};

    EXPECT_EQ(stairHalfOf(cells, cells[0]), StairHalf::Lower);
    EXPECT_EQ(stairHalfOf(cells, cells[1]), StairHalf::Upper);
}



TEST(VoxelStairsTest, StairHalfOf_StandsAVoxelThatIsNoStepAtNoLevel)
{
    using antwika::voxel::stairHalfOf;
    using antwika::voxel::StairHalf;

    const std::vector<VoxelCell> cells{VoxelCell{}};

    EXPECT_EQ(stairHalfOf(cells, cells[0]), StairHalf::Any);
}



TEST(VoxelStairsTest, InferredRampDirection_TakesTheWayAVoxelWasTold)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;
    using antwika::voxel::stepVectorFor;

    for (const auto told :
         {Facing::East, Facing::West, Facing::North, Facing::South})
    {
        const std::vector<VoxelCell> cells{
            VoxelCell{
                .x = 0,
                .y = 0,
                .z = 0,
                .kind = Kind::Ramp,
                .facing = told},
            VoxelCell{.x = 1, .y = 0, .z = 0, .kind = Kind::Normal}};

        EXPECT_EQ(
            inferredRampDirection(cells, cells[0]),
            stepVectorFor(told));
    }
}



TEST(VoxelStairsTest, InferredRampDirection_ReckonsAVoxelToldNothing)
{
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;

    const std::vector<VoxelCell> cells{
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 1, .y = 0, .z = 0, .kind = Kind::Normal}};

    EXPECT_EQ(
        inferredRampDirection(cells, cells[0]), VoxelCell{.x = 1});
}



TEST(VoxelStairsTest, InferredRampDirection_HoldsAToldWayWithNothingBesideIt)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;
    using antwika::voxel::stepVectorFor;

    const std::vector<VoxelCell> cells{
        VoxelCell{
            .x = 0,
            .y = 0,
            .z = 0,
            .kind = Kind::Ramp,
            .facing = Facing::North}};

    EXPECT_EQ(
        inferredRampDirection(cells, cells[0]),
        stepVectorFor(Facing::North));
}



TEST(VoxelStairsTest, StepVectorFor_NamesTheStepEachFacingStandsFor)
{
    using antwika::voxel::Facing;
    using antwika::voxel::facingOfStep;
    using antwika::voxel::stepVectorFor;

    for (const auto facing :
         {Facing::East, Facing::West, Facing::North, Facing::South})
    {
        EXPECT_EQ(facingOfStep(stepVectorFor(facing)), facing);
    }

    EXPECT_EQ(stepVectorFor(Facing::Any), VoxelCell{});
}



TEST(VoxelStairsTest, InferredRampDirection_KeepsAWholeCubeOfOneMind)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;

    for (const auto climbCell :
         {VoxelCell{.x = 1},
          VoxelCell{.x = -1},
          VoxelCell{.z = 1},
          VoxelCell{.z = -1}})
    {
        std::vector<VoxelCell> cells;

        for (const auto cell :
             cubeVoxels(VoxelCell{}, Kind::Ramp, climbCell))
        {
            cells.push_back(cell);
        }

        for (const auto cell :
             cubeVoxels(
                 VoxelCell{.x = 2}, Kind::Ramp, VoxelCell{.x = -1}))
        {
            cells.push_back(cell);
        }

        for (const auto cell : cells)
        {
            if (cell.kind != Kind::Ramp || cell.x > 1)
            {
                continue;
            }

            EXPECT_EQ(inferredRampDirection(cells, cell), climbCell);
        }
    }
}



TEST(
    VoxelStairsTest,
    InferredRampDirection_LeavesARampBesideAnotherToItsOwnShape)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;

    std::vector<VoxelCell> cells;

    for (const auto cell :
         cubeVoxels(VoxelCell{}, Kind::Ramp, VoxelCell{.z = -1}))
    {
        cells.push_back(cell);
    }

    for (const auto cell :
         cubeVoxels(
             VoxelCell{.x = 2}, Kind::Ramp, VoxelCell{.z = -1}))
    {
        cells.push_back(cell);
    }

    for (const auto cell : cells)
    {
        EXPECT_EQ(
            inferredRampDirection(cells, cell), VoxelCell{.z = -1});
    }
}
