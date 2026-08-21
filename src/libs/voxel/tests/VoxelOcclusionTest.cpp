#include <gtest/gtest.h>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <set>

#include "antwika/voxel/VoxelOcclusion.hpp"

using antwika::voxel::VoxelCell;

TEST(VoxelOcclusionTest, OccludingVoxels_HidesNothingWithNothingInTheWay)
{
    using antwika::voxel::occludingVoxels;

    std::set<VoxelCell> filledCells;

    for (std::int32_t x = -2; x <= 2; ++x)
    {
        for (std::int32_t z = -2; z <= 4; ++z)
        {
            filledCells.insert(VoxelCell{.x = x, .y = 0, .z = z});
        }
    }

    EXPECT_TRUE(
        occludingVoxels(filledCells, glm::vec3{0.0F, 0.5F, 0.0F})
            .empty());
}

namespace
{

    constexpr glm::vec3 kStanding{0.0F, 0.5F, 0.0F};

}

namespace
{

    [[nodiscard]] std::set<VoxelCell> groundUnder()
    {
        std::set<VoxelCell> filledCells;

        for (std::int32_t x = -3; x <= 3; ++x)
        {
            for (std::int32_t z = -4; z <= 4; ++z)
            {
                filledCells.insert(VoxelCell{.x = x, .y = 0, .z = z});
            }
        }

        return filledCells;
    }

}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsTheRoofOverTheHeadWholeBothWays)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = groundUnder();

    for (std::int32_t z = -4; z <= 4; ++z)
    {
        filledCells.insert(VoxelCell{.x = 0, .y = 3, .z = z});
    }

    const auto occludingCells =
        occludingVoxels(filledCells, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = -4}));
    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = 3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheRoofStandingFarAcrossFromTheHead)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = groundUnder();

    for (std::int32_t x = -20; x <= 20; ++x)
    {
        for (std::int32_t z = -20; z <= 20; ++z)
        {
            filledCells.insert(VoxelCell{.x = x, .y = 3, .z = z});
        }
    }

    const auto occludingCells =
        occludingVoxels(filledCells, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 12, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = 20, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = -20, .y = 3, .z = 0}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsARoofItIsCutOffFromOverTheHead)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = groundUnder();

    filledCells.insert(VoxelCell{.x = 0, .y = 3, .z = 0});
    filledCells.insert(VoxelCell{.x = 5, .y = 3, .z = 0});

    const auto occludingCells =
        occludingVoxels(filledCells, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 5, .y = 3, .z = 0}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatStandsUnderTheRoofItLifts)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = groundUnder();

    for (std::int32_t x = -3; x <= 3; ++x)
    {
        filledCells.insert(VoxelCell{.x = x, .y = 3, .z = 0});
    }

    filledCells.insert(VoxelCell{.x = 3, .y = 1, .z = 0});
    filledCells.insert(VoxelCell{.x = 3, .y = 2, .z = 0});

    const auto occludingCells =
        occludingVoxels(filledCells, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 3, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = 3, .y = 2, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = 3, .y = 1, .z = 0}));
}

namespace
{

    [[nodiscard]] std::set<VoxelCell> roomShutIn()
    {
        std::set<VoxelCell> filledCells;

        for (std::int32_t x = -8; x <= 8; ++x)
        {
            for (std::int32_t z = -8; z <= 8; ++z)
            {
                filledCells.insert(VoxelCell{.x = x, .y = 0, .z = z});
            }
        }

        for (std::int32_t y = 1; y <= 4; ++y)
        {
            for (std::int32_t sideIndex = -4; sideIndex <= 4; ++sideIndex)
            {
                filledCells.insert(VoxelCell{.x = -4, .y = y, .z = sideIndex});
                filledCells.insert(VoxelCell{.x = 4, .y = y, .z = sideIndex});
                filledCells.insert(VoxelCell{.x = sideIndex, .y = y, .z = -4});
                filledCells.insert(VoxelCell{.x = sideIndex, .y = y, .z = 4});
            }
        }

        for (std::int32_t x = -4; x <= 4; ++x)
        {
            for (std::int32_t z = -4; z <= 4; ++z)
            {
                filledCells.insert(VoxelCell{.x = x, .y = 5, .z = z});
            }
        }

        return filledCells;
    }

}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsTheFloorOfTheStoreyOverTheWallsAbout)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = roomShutIn();

    const VoxelCell storeyOverCell{.x = 7, .y = 3, .z = -7};

    filledCells.insert(storeyOverCell);

    const auto occludingCells =
        occludingVoxels(filledCells, kStanding);

    EXPECT_TRUE(occludingCells.contains(storeyOverCell));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheWallsOfTheRoomItStandsIn)
{
    using antwika::voxel::occludingVoxels;

    const auto occludingCells =
        occludingVoxels(roomShutIn(), kStanding);

    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = -4, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = -4, .y = 4, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = -4}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheGroundUnderTheRoomItLiftsOver)
{
    using antwika::voxel::occludingVoxels;

    const auto occludingCells =
        occludingVoxels(roomShutIn(), kStanding);

    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = 0, .y = 0, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = 7, .y = 0, .z = -7}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheRoofStandingFarSouthOfTheHead)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = groundUnder();

    for (std::int32_t z = -20; z <= 20; ++z)
    {
        filledCells.insert(VoxelCell{.x = 0, .y = 3, .z = z});
    }

    const auto occludingCells =
        occludingVoxels(filledCells, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = 18}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatARoofRestsOnStandingBelowIt)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = groundUnder();

    for (std::int32_t z = -4; z <= 4; ++z)
    {
        filledCells.insert(VoxelCell{.x = 0, .y = 3, .z = z});
    }

    const VoxelCell holdingItUpCell{.x = 0, .y = 2, .z = -3};

    filledCells.insert(holdingItUpCell);
    filledCells.insert(VoxelCell{.x = 0, .y = 1, .z = -3});

    const auto occludingCells =
        occludingVoxels(filledCells, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelCell{.x = 0, .y = 3, .z = -3}));
    EXPECT_FALSE(occludingCells.contains(holdingItUpCell));
    EXPECT_FALSE(occludingCells.contains(VoxelCell{.x = 0, .y = 1, .z = -3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsNoRoofOverTheReachOfTheSightPoint)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = groundUnder();

    for (std::int32_t x = -3; x <= 3; ++x)
    {
        for (std::int32_t z = -4; z <= 4; ++z)
        {
            filledCells.insert(VoxelCell{.x = x, .y = 9, .z = z});
        }
    }

    EXPECT_TRUE(occludingVoxels(filledCells, kStanding).empty());
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsNoRoofWhereTheSkyIsOverTheHead)
{
    using antwika::voxel::occludingVoxels;

    auto filledCells = groundUnder();
    const VoxelCell asideCell{.x = 2, .y = 3, .z = -3};

    filledCells.insert(asideCell);

    const auto occludingCells =
        occludingVoxels(filledCells, kStanding);

    EXPECT_FALSE(occludingCells.contains(asideCell));
}

