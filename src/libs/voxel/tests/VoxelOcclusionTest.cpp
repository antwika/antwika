#include <gtest/gtest.h>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>

#include "antwika/voxel/VoxelOcclusion.hpp"

using antwika::voxel::VoxelMaterial;
using antwika::voxel::VoxelPosition;
using antwika::voxel::Voxels;

TEST(VoxelOcclusionTest, OccludingVoxels_HidesNothingWithNothingInTheWay)
{
    using antwika::voxel::occludingVoxels;

    Voxels filledVoxels;

    for (std::int32_t x = -2; x <= 2; ++x)
    {
        for (std::int32_t z = -2; z <= 4; ++z)
        {
            filledVoxels[VoxelPosition{.x = x, .y = 0, .z = z}] =
                VoxelMaterial{};
        }
    }

    EXPECT_TRUE(
        occludingVoxels(filledVoxels, glm::vec3{0.0F, 0.5F, 0.0F})
            .empty());
}

namespace
{

    constexpr glm::vec3 kStanding{0.0F, 0.5F, 0.0F};

}

namespace
{

    [[nodiscard]] Voxels groundUnder()
    {
        Voxels filledVoxels;

        for (std::int32_t x = -3; x <= 3; ++x)
        {
            for (std::int32_t z = -4; z <= 4; ++z)
            {
                filledVoxels[VoxelPosition{.x = x, .y = 0, .z = z}] =
                VoxelMaterial{};
            }
        }

        return filledVoxels;
    }

}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsTheRoofOverTheHeadWholeBothWays)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = groundUnder();

    for (std::int32_t z = -4; z <= 4; ++z)
    {
        filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = z}] =
                VoxelMaterial{};
    }

    const auto occludingCells =
        occludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3,
        .z = -4}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheRoofStandingFarAcrossFromTheHead)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = groundUnder();

    for (std::int32_t x = -20; x <= 20; ++x)
    {
        for (std::int32_t z = -20; z <= 20; ++z)
        {
            filledVoxels[VoxelPosition{.x = x, .y = 3, .z = z}] =
                VoxelMaterial{};
        }
    }

    const auto occludingCells =
        occludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 12, .y = 3,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 20, .y = 3,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = -20, .y = 3,
        .z = 0}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsARoofItIsCutOffFromOverTheHead)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = groundUnder();

    filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = 0}] =
                VoxelMaterial{};
    filledVoxels[VoxelPosition{.x = 5, .y = 3, .z = 0}] =
                VoxelMaterial{};

    const auto occludingCells =
        occludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 5, .y = 3, .z = 0}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatStandsUnderTheRoofItLifts)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = groundUnder();

    for (std::int32_t x = -3; x <= 3; ++x)
    {
        filledVoxels[VoxelPosition{.x = x, .y = 3, .z = 0}] =
                VoxelMaterial{};
    }

    filledVoxels[VoxelPosition{.x = 3, .y = 1, .z = 0}] =
                VoxelMaterial{};
    filledVoxels[VoxelPosition{.x = 3, .y = 2, .z = 0}] =
                VoxelMaterial{};

    const auto occludingCells =
        occludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 3, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 3, .y = 2,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 3, .y = 1,
        .z = 0}));
}

namespace
{

    [[nodiscard]] Voxels roomShutIn()
    {
        Voxels filledVoxels;

        for (std::int32_t x = -8; x <= 8; ++x)
        {
            for (std::int32_t z = -8; z <= 8; ++z)
            {
                filledVoxels[VoxelPosition{.x = x, .y = 0, .z = z}] =
                VoxelMaterial{};
            }
        }

        for (std::int32_t y = 1; y <= 4; ++y)
        {
            for (std::int32_t sideIndex = -4; sideIndex <= 4; ++sideIndex)
            {
                filledVoxels[VoxelPosition{.x = -4, .y = y, .z = sideIndex}] =
                VoxelMaterial{};
                filledVoxels[VoxelPosition{.x = 4, .y = y, .z = sideIndex}] =
                VoxelMaterial{};
                filledVoxels[VoxelPosition{.x = sideIndex, .y = y, .z = -4}] =
                VoxelMaterial{};
                filledVoxels[VoxelPosition{.x = sideIndex, .y = y, .z = 4}] =
                VoxelMaterial{};
            }
        }

        for (std::int32_t x = -4; x <= 4; ++x)
        {
            for (std::int32_t z = -4; z <= 4; ++z)
            {
                filledVoxels[VoxelPosition{.x = x, .y = 5, .z = z}] =
                VoxelMaterial{};
            }
        }

        return filledVoxels;
    }

}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsTheFloorOfTheStoreyOverTheWallsAbout)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = roomShutIn();

    const VoxelPosition storeyOverPosition{.x = 7, .y = 3, .z = -7};

    filledVoxels[storeyOverPosition] = VoxelMaterial{};

    const auto occludingCells =
        occludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(storeyOverPosition));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheWallsOfTheRoomItStandsIn)
{
    using antwika::voxel::occludingVoxels;

    const auto occludingCells =
        occludingVoxels(roomShutIn(), kStanding);

    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = -4, .y = 3,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = -4, .y = 4,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3,
        .z = -4}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheGroundUnderTheRoomItLiftsOver)
{
    using antwika::voxel::occludingVoxels;

    const auto occludingCells =
        occludingVoxels(roomShutIn(), kStanding);

    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 0, .y = 0,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 7, .y = 0,
        .z = -7}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheRoofStandingFarSouthOfTheHead)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = groundUnder();

    for (std::int32_t z = -20; z <= 20; ++z)
    {
        filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = z}] =
                VoxelMaterial{};
    }

    const auto occludingCells =
        occludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3,
        .z = 18}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatARoofRestsOnStandingBelowIt)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = groundUnder();

    for (std::int32_t z = -4; z <= 4; ++z)
    {
        filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = z}] =
                VoxelMaterial{};
    }

    const VoxelPosition holdingItUpPosition{.x = 0, .y = 2, .z = -3};

    filledVoxels[holdingItUpPosition] = VoxelMaterial{};
    filledVoxels[VoxelPosition{.x = 0, .y = 1, .z = -3}] =
                VoxelMaterial{};

    const auto occludingCells =
        occludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3,
        .z = -3}));
    EXPECT_FALSE(occludingCells.contains(holdingItUpPosition));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 0, .y = 1,
        .z = -3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsNoRoofOverTheReachOfTheSightPoint)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = groundUnder();

    for (std::int32_t x = -3; x <= 3; ++x)
    {
        for (std::int32_t z = -4; z <= 4; ++z)
        {
            filledVoxels[VoxelPosition{.x = x, .y = 9, .z = z}] =
                VoxelMaterial{};
        }
    }

    EXPECT_TRUE(occludingVoxels(filledVoxels, kStanding).empty());
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsNoRoofWhereTheSkyIsOverTheHead)
{
    using antwika::voxel::occludingVoxels;

    auto filledVoxels = groundUnder();
    const VoxelPosition asidePosition{.x = 2, .y = 3, .z = -3};

    filledVoxels[asidePosition] = VoxelMaterial{};

    const auto occludingCells =
        occludingVoxels(filledVoxels, kStanding);

    EXPECT_FALSE(occludingCells.contains(asidePosition));
}

TEST(VoxelOcclusionTest, VoxelUnder_TakesTheCellThePointFallsIn)
{
    using antwika::voxel::voxelUnder;

    EXPECT_EQ(
        voxelUnder(glm::vec3{0.5F, 3.5F, -0.5F}),
        (VoxelPosition{.x = 0, .y = 3, .z = -1}));
}

namespace
{

    constexpr float kSightClearance = 0.375F;

}

TEST(VoxelOcclusionTest, CubeAbove_FindsNothingUnderTheOpenSky)
{
    using antwika::voxel::cubeAbove;

    EXPECT_FALSE(cubeAbove(groundUnder(), kStanding, kSightClearance));
}

TEST(VoxelOcclusionTest, CubeAbove_FindsTheCubeOverTheHead)
{
    using antwika::voxel::cubeAbove;

    auto filledVoxels = groundUnder();

    filledVoxels[VoxelPosition{.x = 1, .y = 2, .z = 1}] = VoxelMaterial{};

    EXPECT_TRUE(cubeAbove(filledVoxels, kStanding, kSightClearance));
}

TEST(VoxelOcclusionTest, CubeAbove_FindsNothingWhereTheWaterIs)
{
    using antwika::voxel::cubeAbove;

    auto filledVoxels = groundUnder();

    filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = 0}] =
        VoxelMaterial{.kind = antwika::voxel::Kind::Water};

    EXPECT_FALSE(cubeAbove(filledVoxels, kStanding, kSightClearance));
}

TEST(VoxelOcclusionTest, CubeAbove_FindsTheCubeTheClearanceReaches)
{
    using antwika::voxel::cubeAbove;

    auto filledVoxels = groundUnder();

    filledVoxels[VoxelPosition{.x = 0, .y = 2, .z = 2}] = VoxelMaterial{};

    const glm::vec3 aboutToEnter{0.0F, 0.5F, 1.9F};

    EXPECT_FALSE(cubeAbove(filledVoxels, aboutToEnter, 0.0F));
    EXPECT_TRUE(cubeAbove(filledVoxels, aboutToEnter, kSightClearance));
}

TEST(VoxelOcclusionTest, CubeAbove_LeavesTheCubeAClearWalkAwayAlone)
{
    using antwika::voxel::cubeAbove;

    auto filledVoxels = groundUnder();

    filledVoxels[VoxelPosition{.x = 0, .y = 2, .z = 2}] = VoxelMaterial{};

    EXPECT_FALSE(
        cubeAbove(
            filledVoxels,
            glm::vec3{0.0F, 0.5F, 1.0F},
            kSightClearance));
}
