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
    using antwika::voxel::getOccludingVoxels;

    Voxels filledVoxels;

    for (std::int32_t x = -2; x <= 2; ++x)
    {
        for (std::int32_t z = -2; z <= 4; ++z)
        {
            filledVoxels[VoxelPosition{.x = x, .y = -1, .z = z}] =
                VoxelMaterial{};
        }
    }

    EXPECT_TRUE(
        getOccludingVoxels(filledVoxels, glm::vec3{0.0F, 0.5F, 0.0F})
            .empty());
}

namespace
{

    constexpr glm::vec3 kStanding{0.0F, 0.5F, 0.0F};

}

namespace
{

    [[nodiscard]] Voxels getGroundUnder()
    {
        Voxels filledVoxels;

        for (std::int32_t x = -3; x <= 3; ++x)
        {
            for (std::int32_t z = -4; z <= 4; ++z)
            {
                filledVoxels[VoxelPosition{.x = x, .y = -1, .z = z}] =
                VoxelMaterial{};
            }
        }

        return filledVoxels;
    }

}

TEST(VoxelOcclusionTest, OccludingVoxels_LiftsWhatStandsOverTheHead)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();
    const VoxelPosition overheadPosition{.x = 0, .y = 4, .z = 0};

    filledVoxels[overheadPosition] = VoxelMaterial{};

    EXPECT_TRUE(
        getOccludingVoxels(filledVoxels, kStanding)
            .contains(overheadPosition));
}

TEST(VoxelOcclusionTest, OccludingVoxels_LeavesWhatStandsAtItsOwnCubeLevel)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();
    const VoxelPosition besidePosition{.x = 1, .y = 1, .z = 1};

    filledVoxels[besidePosition] = VoxelMaterial{};

    EXPECT_FALSE(
        getOccludingVoxels(filledVoxels, kStanding)
            .contains(besidePosition));
}

TEST(VoxelOcclusionTest, OccludingVoxels_LeavesTheWaterOverTheHead)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();
    const VoxelPosition overheadPosition{.x = 0, .y = 4, .z = 0};

    filledVoxels[overheadPosition] =
        VoxelMaterial{.kind = antwika::voxel::Kind::Water};

    EXPECT_FALSE(
        getOccludingVoxels(filledVoxels, kStanding)
            .contains(overheadPosition));
}

TEST(VoxelOcclusionTest, OccludingVoxels_TakesTheWallItSpreadsIntoSideways)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();
    const VoxelPosition asidePosition{.x = 4, .y = 4, .z = 0};

    filledVoxels[VoxelPosition{.x = 3, .y = 4, .z = 0}] = VoxelMaterial{};
    filledVoxels[asidePosition] = VoxelMaterial{};

    EXPECT_TRUE(
        getOccludingVoxels(filledVoxels, kStanding).contains(asidePosition));
}

TEST(VoxelOcclusionTest, OccludingVoxels_TakesTheWallItSpreadsIntoUpwards)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    filledVoxels[VoxelPosition{.x = 3, .y = 4, .z = 0}] = VoxelMaterial{};

    for (std::int32_t y = 4; y <= 9; ++y)
    {
        filledVoxels[VoxelPosition{.x = 4, .y = y, .z = 0}] = VoxelMaterial{};
    }

    EXPECT_TRUE(
        getOccludingVoxels(filledVoxels, kStanding)
            .contains(VoxelPosition{.x = 4, .y = 9, .z = 0}));
}

TEST(VoxelOcclusionTest, OccludingVoxels_LeavesTheWallUnderWhatItLifts)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    for (std::int32_t x = 0; x <= 5; ++x)
    {
        filledVoxels[VoxelPosition{.x = x, .y = 8, .z = 0}] = VoxelMaterial{};
    }

    for (std::int32_t y = 2; y <= 7; ++y)
    {
        filledVoxels[VoxelPosition{.x = 5, .y = y, .z = 0}] = VoxelMaterial{};
    }

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 5, .y = 8, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 5, .y = 7,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 5, .y = 2,
        .z = 0}));
}

namespace
{

    [[nodiscard]] bool isLoneCubeLifted(const VoxelPosition position)
    {
        using antwika::voxel::getOccludingVoxels;

        auto filledVoxels = getGroundUnder();

        filledVoxels[position] = VoxelMaterial{};

        return getOccludingVoxels(filledVoxels, kStanding).contains(position);
    }

}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsWhatStandsOverTheCubeBeforeItAcross)
{
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = -2, .y = 4, .z = 0}));
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = -1, .y = 4, .z = 1}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsWhatStandsOverTheCubeAfterItAcross)
{
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 2, .y = 4, .z = 0}));
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 3, .y = 4, .z = 1}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsWhatStandsOverTheCubeAfterItAlong)
{
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 0, .y = 4, .z = 2}));
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 1, .y = 4, .z = 3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsWhatStandsOverTheCubeTwoStepsAfterItAlong)
{
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 0, .y = 4, .z = 4}));
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 1, .y = 4, .z = 5}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsWhatStandsTwoCubesOverTheThirdStepAlong)
{
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 0, .y = 4, .z = 6}));
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 1, .y = 5, .z = 7}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatStandsOneCubeOverTheThirdStepAlong)
{
    EXPECT_FALSE(isLoneCubeLifted(VoxelPosition{.x = 0, .y = 2, .z = 6}));
    EXPECT_FALSE(isLoneCubeLifted(VoxelPosition{.x = 1, .y = 3, .z = 7}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatStandsOverTheCubeFourStepsAfterItAlong)
{
    EXPECT_FALSE(isLoneCubeLifted(VoxelPosition{.x = 0, .y = 6, .z = 8}));
    EXPECT_FALSE(isLoneCubeLifted(VoxelPosition{.x = 1, .y = 6, .z = 9}));
}

namespace
{

    [[nodiscard]] bool isLoneCubeLiftedBeside(
        const VoxelPosition position, const VoxelPosition besidePosition)
    {
        using antwika::voxel::getOccludingVoxels;

        auto filledVoxels = getGroundUnder();

        filledVoxels[position] = VoxelMaterial{};
        filledVoxels[besidePosition] = VoxelMaterial{};

        return getOccludingVoxels(filledVoxels, kStanding).contains(position);
    }

}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheCubeBeforeItAcrossItCannotWalkUnder)
{
    EXPECT_FALSE(
        isLoneCubeLiftedBeside(
            VoxelPosition{.x = -2, .y = 4, .z = 0},
            VoxelPosition{.x = -2, .y = 0, .z = 0}));
    EXPECT_FALSE(
        isLoneCubeLiftedBeside(
            VoxelPosition{.x = -2, .y = 4, .z = 0},
            VoxelPosition{.x = -1, .y = 1, .z = 1}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheCubeAfterItAcrossItCannotWalkUnder)
{
    EXPECT_FALSE(
        isLoneCubeLiftedBeside(
            VoxelPosition{.x = 2, .y = 4, .z = 0},
            VoxelPosition{.x = 3, .y = 1, .z = 1}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsOverACubeAcrossThatOnlyStandsAboveTheWalker)
{
    EXPECT_TRUE(
        isLoneCubeLiftedBeside(
            VoxelPosition{.x = 2, .y = 4, .z = 0},
            VoxelPosition{.x = 2, .y = 2, .z = 0}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsOverTheCubesAlongItEvenWhenItCannotWalkUnder)
{
    EXPECT_TRUE(
        isLoneCubeLiftedBeside(
            VoxelPosition{.x = 0, .y = 4, .z = 2},
            VoxelPosition{.x = 0, .y = 0, .z = 2}));
    EXPECT_TRUE(
        isLoneCubeLiftedBeside(
            VoxelPosition{.x = -2, .y = 4, .z = 2},
            VoxelPosition{.x = -2, .y = 0, .z = 2}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsWhatStandsOverTheCubeBeforeItAcrossAndAfterItAlong)
{
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = -2, .y = 4, .z = 2}));
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = -1, .y = 4, .z = 3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsWhatStandsOverTheCubeAfterItAcrossAndAlong)
{
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 2, .y = 4, .z = 2}));
    EXPECT_TRUE(isLoneCubeLifted(VoxelPosition{.x = 3, .y = 4, .z = 3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatStandsOverTheCubesBesideItBeforeItAlong)
{
    EXPECT_FALSE(isLoneCubeLifted(VoxelPosition{.x = -2, .y = 4, .z = -2}));
    EXPECT_FALSE(isLoneCubeLifted(VoxelPosition{.x = 3, .y = 4, .z = -1}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatStandsOverTheCubeBeforeItAlong)
{
    EXPECT_FALSE(isLoneCubeLifted(VoxelPosition{.x = 0, .y = 4, .z = -2}));
    EXPECT_FALSE(isLoneCubeLifted(VoxelPosition{.x = 1, .y = 4, .z = -1}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsTheRoofOverTheHeadWholeBothWays)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    for (std::int32_t z = -4; z <= 4; ++z)
    {
        filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = z}] =
                VoxelMaterial{};
    }

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3,
        .z = -4}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheRoofStandingFarAcrossFromTheHead)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    for (std::int32_t x = -20; x <= 20; ++x)
    {
        for (std::int32_t z = -20; z <= 20; ++z)
        {
            filledVoxels[VoxelPosition{.x = x, .y = 3, .z = z}] =
                VoxelMaterial{};
        }
    }

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

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
    OccludingVoxels_TakesTheRoofToTheRimOfTheMaskWindow)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    for (std::int32_t x = -20; x <= 20; ++x)
    {
        filledVoxels[VoxelPosition{.x = x, .y = 3, .z = 0}] =
                VoxelMaterial{};
    }

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = -16, .y = 3,
        .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 15, .y = 3,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 16, .y = 3,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = -17, .y = 3,
        .z = 0}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesARoofCutOffFromTheOneOverTheHead)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = 0}] =
                VoxelMaterial{};
    filledVoxels[VoxelPosition{.x = 5, .y = 3, .z = 0}] =
                VoxelMaterial{};

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 5, .y = 3,
        .z = 0}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatStandsUnderTheRoofItLifts)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    for (std::int32_t x = -7; x <= 7; ++x)
    {
        filledVoxels[VoxelPosition{.x = x, .y = 3, .z = 0}] =
                VoxelMaterial{};
    }

    filledVoxels[VoxelPosition{.x = 7, .y = 1, .z = 0}] =
                VoxelMaterial{};
    filledVoxels[VoxelPosition{.x = 7, .y = 2, .z = 0}] =
                VoxelMaterial{};

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 7, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 7, .y = 2,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 7, .y = 1,
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
                filledVoxels[VoxelPosition{.x = x, .y = -1, .z = z}] =
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
    OccludingVoxels_LeavesTheStoreyFloorNothingJoinsToTheRoof)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = roomShutIn();

    const VoxelPosition storeyOverPosition{.x = 7, .y = 3, .z = -7};

    filledVoxels[storeyOverPosition] = VoxelMaterial{};

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_FALSE(occludingCells.contains(storeyOverPosition));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsTheWallsOfTheRoomOverTheCubeItStandsIn)
{
    using antwika::voxel::getOccludingVoxels;

    const auto occludingCells =
        getOccludingVoxels(roomShutIn(), kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 4}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = -4, .y = 3,
        .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3,
        .z = -4}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheWallsOfTheRoomAtTheCubeItStandsIn)
{
    using antwika::voxel::getOccludingVoxels;

    const auto occludingCells =
        getOccludingVoxels(roomShutIn(), kStanding);

    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = -4, .y = 1,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 0, .y = 1,
        .z = -4}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheGroundUnderTheRoomItLiftsOver)
{
    using antwika::voxel::getOccludingVoxels;

    const auto occludingCells =
        getOccludingVoxels(roomShutIn(), kStanding);

    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 0, .y = -1,
        .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 7, .y = -1,
        .z = -7}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesTheRoofStandingFarSouthOfTheHead)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    for (std::int32_t z = -20; z <= 20; ++z)
    {
        filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = z}] =
                VoxelMaterial{};
    }

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3, .z = 0}));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3,
        .z = 18}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LeavesWhatARoofRestsOnStandingBelowIt)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

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
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 3,
        .z = -3}));
    EXPECT_FALSE(occludingCells.contains(holdingItUpPosition));
    EXPECT_FALSE(occludingCells.contains(VoxelPosition{.x = 0, .y = 1,
        .z = -3}));
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsTheRoofHoweverHighItStands)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();

    for (std::int32_t x = -3; x <= 3; ++x)
    {
        for (std::int32_t z = -4; z <= 4; ++z)
        {
            filledVoxels[VoxelPosition{.x = x, .y = 9, .z = z}] =
                VoxelMaterial{};
        }
    }

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 0, .y = 9, .z = 0}));
    EXPECT_TRUE(occludingCells.contains(VoxelPosition{.x = 3, .y = 9, .z = 4}));
}

TEST(VoxelOcclusionTest, OccludingVoxels_LiftsNothingFromUnderTheMaskFloor)
{
    using antwika::voxel::getOccludingVoxels;

    Voxels filledVoxels;

    filledVoxels[VoxelPosition{.x = 0, .y = -4, .z = 0}] = VoxelMaterial{};

    EXPECT_TRUE(
        getOccludingVoxels(filledVoxels, glm::vec3{0.0F, -7.5F, 0.0F})
            .empty());
}

TEST(VoxelOcclusionTest, OccludingVoxels_LiftsNoMoreThanTheMaskHolds)
{
    using antwika::voxel::getOccludingVoxels;
    using antwika::voxel::kMaxOccludedVoxels;
    using antwika::voxel::kOcclusionMaskLevels;
    using antwika::voxel::kOcclusionMaskWidth;

    Voxels filledVoxels;
    const auto arm = static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

    for (std::int32_t x = -arm; x < arm; ++x)
    {
        for (std::int32_t z = -arm; z < arm; ++z)
        {
            for (std::int32_t y = 2;
                 y < static_cast<std::int32_t>(kOcclusionMaskLevels);
                 ++y)
            {
                filledVoxels[VoxelPosition{.x = x, .y = y, .z = z}] =
                VoxelMaterial{};
            }
        }
    }

    EXPECT_EQ(
        getOccludingVoxels(filledVoxels, kStanding).size(),
        kMaxOccludedVoxels);
}

TEST(
    VoxelOcclusionTest,
    OccludingVoxels_LiftsNoRoofWhereTheSkyIsOverTheHead)
{
    using antwika::voxel::getOccludingVoxels;

    auto filledVoxels = getGroundUnder();
    const VoxelPosition asidePosition{.x = 2, .y = 3, .z = -3};

    filledVoxels[asidePosition] = VoxelMaterial{};

    const auto occludingCells =
        getOccludingVoxels(filledVoxels, kStanding);

    EXPECT_FALSE(occludingCells.contains(asidePosition));
}

TEST(VoxelOcclusionTest, VoxelUnder_TakesTheCellThePointFallsIn)
{
    using antwika::voxel::getVoxelUnder;

    EXPECT_EQ(
        getVoxelUnder(glm::vec3{0.5F, 3.5F, -0.5F}),
        (VoxelPosition{.x = 0, .y = 3, .z = -1}));
}

namespace
{

    constexpr float kSightClearance = 0.375F;

}

TEST(VoxelOcclusionTest, CubeAbove_FindsNothingUnderTheOpenSky)
{
    using antwika::voxel::isCubeAbove;

    EXPECT_FALSE(isCubeAbove(getGroundUnder(), kStanding, kSightClearance));
}

TEST(VoxelOcclusionTest, CubeAbove_FindsTheCubeOverTheHead)
{
    using antwika::voxel::isCubeAbove;

    auto filledVoxels = getGroundUnder();

    filledVoxels[VoxelPosition{.x = 1, .y = 2, .z = 1}] = VoxelMaterial{};

    EXPECT_TRUE(isCubeAbove(filledVoxels, kStanding, kSightClearance));
}

TEST(VoxelOcclusionTest, CubeAbove_FindsNothingWhereTheWaterIs)
{
    using antwika::voxel::isCubeAbove;

    auto filledVoxels = getGroundUnder();

    filledVoxels[VoxelPosition{.x = 0, .y = 3, .z = 0}] =
        VoxelMaterial{.kind = antwika::voxel::Kind::Water};

    EXPECT_FALSE(isCubeAbove(filledVoxels, kStanding, kSightClearance));
}

TEST(VoxelOcclusionTest, CubeAbove_FindsTheCubeTheClearanceReaches)
{
    using antwika::voxel::isCubeAbove;

    auto filledVoxels = getGroundUnder();

    filledVoxels[VoxelPosition{.x = 0, .y = 2, .z = 2}] = VoxelMaterial{};

    const glm::vec3 aboutToEnter{0.0F, 0.5F, 1.9F};

    EXPECT_FALSE(isCubeAbove(filledVoxels, aboutToEnter, 0.0F));
    EXPECT_TRUE(isCubeAbove(filledVoxels, aboutToEnter, kSightClearance));
}

TEST(VoxelOcclusionTest, CubeAbove_LeavesTheCubeAClearWalkAwayAlone)
{
    using antwika::voxel::isCubeAbove;

    auto filledVoxels = getGroundUnder();

    filledVoxels[VoxelPosition{.x = 0, .y = 2, .z = 2}] = VoxelMaterial{};

    EXPECT_FALSE(
        isCubeAbove(
            filledVoxels,
            glm::vec3{0.0F, 0.5F, 1.0F},
            kSightClearance));
}
