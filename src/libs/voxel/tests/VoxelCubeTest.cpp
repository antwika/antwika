#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <set>
#include <vector>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelStairs.hpp>

namespace antwika::voxel
{

    namespace
    {
        TEST(VoxelCubeTest, CubeSide_IsTwoVoxelsEveryWay)
        {
            EXPECT_EQ(kCubeSide, 2);
            EXPECT_EQ(kCubeVoxels, 8U);
        }

        TEST(VoxelCubeTest, CubeTop_StandsTheLowestCubeAtTheGround)
        {
            EXPECT_EQ(cubeTop(0), kCubeSide - 1);
        }

        TEST(VoxelCubeTest, CubeTop_RisesOneCubeAtATime)
        {
            for (std::int32_t cube = -4; cube < 8; ++cube)
            {
                EXPECT_EQ(cubeTop(cube + 1) - cubeTop(cube), kCubeSide);
            }
        }

        TEST(VoxelCubeTest, CubeIndexOfLevel_TakesBackWhatCubeTopGave)
        {
            for (std::int32_t cube = -4; cube < 8; ++cube)
            {
                EXPECT_EQ(cubeIndexOfLevel(cubeTop(cube)), cube);
            }
        }

        TEST(VoxelCubeTest, CubeIndexOfLevel_HoldsEveryLevelOfOneCube)
        {
            for (std::int32_t level = -8; level < 16; ++level)
            {
                const auto cube = cubeIndexOfLevel(level);

                EXPECT_LE(cubeTop(cube) - (kCubeSide - 1), level);
                EXPECT_GE(cubeTop(cube), level);
            }
        }

        TEST(VoxelCubeTest, CubeIndexOfLevel_CountsOnDownUnderTheGround)
        {
            EXPECT_EQ(cubeIndexOfLevel(0), 0);
            EXPECT_EQ(cubeIndexOfLevel(-1), -1);
            EXPECT_EQ(cubeIndexOfLevel(-2), -1);
            EXPECT_EQ(cubeIndexOfLevel(-3), -2);
        }

        TEST(VoxelCubeTest, CubeCells_HoldsAsManyVoxelsAsACubeHas)
        {
            EXPECT_EQ(cubeCells(VoxelPosition{}).size(), kCubeVoxels);
        }

        TEST(VoxelCubeTest, CubeCells_HoldsNoVoxelTwice)
        {
            const auto voxels = cubeCells(VoxelPosition{.x = 4, .y = -2});
            const std::set<VoxelPosition> apartPositions(
                voxels.begin(), voxels.end());

            EXPECT_EQ(apartPositions.size(), voxels.size());
        }

        TEST(VoxelCubeTest, CubeCells_ReachesOneSideFromItsCorner)
        {
            const VoxelPosition cornerPosition{.x = 6, .y = -4, .z = 2};

            for (const auto cell : cubeCells(cornerPosition))
            {
                EXPECT_GE(cell.x, cornerPosition.x);
                EXPECT_GE(cell.y, cornerPosition.y);
                EXPECT_GE(cell.z, cornerPosition.z);
                EXPECT_LT(cell.x, cornerPosition.x + kCubeSide);
                EXPECT_LT(cell.y, cornerPosition.y + kCubeSide);
                EXPECT_LT(cell.z, cornerPosition.z + kCubeSide);
            }
        }

        TEST(VoxelCubeTest, CubeCornerOf_LaysTheVoxelsOfACubeInThatCube)
        {
            const VoxelPosition cornerPosition{.x = -6, .y = 2, .z = 4};

            for (const auto cell : cubeCells(cornerPosition))
            {
                EXPECT_EQ(cubeCornerOf(cell), cornerPosition);
            }
        }

        TEST(VoxelCubeTest, CubeCornerOf_NamesACubeByItsLowestCorner)
        {
            EXPECT_EQ(
                cubeCornerOf(VoxelPosition{.x = 3, .y = 2, .z = 1}),
                (VoxelPosition{.x = 2, .y = 2, .z = 0}));
        }

        TEST(VoxelCubeTest, CubeCornerOf_LaysAVoxelBelowNoughtInTheCubeBelowIt)
        {
            EXPECT_EQ(
                cubeCornerOf(VoxelPosition{.x = -1, .y = -2, .z = -3}),
                (VoxelPosition{.x = -2, .y = -2, .z = -4}));
        }

        TEST(VoxelCubeTest, CubeCornerOf_LeavesNoVoxelBetweenNeighbouringCubes)
        {
            const auto hereCells =
                cubeCells(cubeCornerOf(VoxelPosition{.x = 0}));
            const auto thereCells =
                cubeCells(cubeCornerOf(VoxelPosition{.x = kCubeSide}));
            std::set<VoxelPosition> bothPositions(hereCells.begin(),
                hereCells.end());

            bothPositions.insert(thereCells.begin(), thereCells.end());

            EXPECT_EQ(bothPositions.size(), kCubeVoxels * 2);
            EXPECT_TRUE(bothPositions.contains(VoxelPosition{.x = 1}));
            EXPECT_TRUE(bothPositions.contains(VoxelPosition{.x = kCubeSide}));
        }

        TEST(VoxelCubeTest, ExpandCubesToVoxels_GivesAWholeCubePerCell)
        {
            const auto voxels =
                voxelsOf({VoxelCell{}, VoxelCell{.x = 1}});

            EXPECT_EQ(
                expandCubesToVoxels(voxels).size(),
                voxels.size() * kCubeVoxels);
        }

        TEST(VoxelCubeTest, ExpandCubesToVoxels_LetsNoTwoBlocksOverlap)
        {
            const auto cubeVoxels = voxelsOf({
                VoxelCell{},
                VoxelCell{.x = 1},
                VoxelCell{.y = 1},
                VoxelCell{.x = -1, .z = 2}});
            const auto voxels = expandCubesToVoxels(cubeVoxels);

            EXPECT_EQ(voxels.size(), cubeVoxels.size() * kCubeVoxels);
        }

        TEST(VoxelCubeTest, ExpandCubesToVoxels_LaysEveryVoxelOfACellInOneCube)
        {
            const auto cubeVoxels = voxelsOf({VoxelCell{.x = 2, .y = -1}});
            const auto voxels = expandCubesToVoxels(cubeVoxels);
            std::set<VoxelPosition> cubePositions;

            for (const auto &[position, material] : voxels)
            {
                cubePositions.insert(cubeCornerOf(position));
            }

            EXPECT_EQ(cubePositions.size(), 1U);
        }

        TEST(
    VoxelCubeTest,
    ExpandCubesToVoxels_KeepsNeighbouringCellsNeighbours)
        {
            const auto voxels = expandCubesToVoxels(
                voxelsOf({VoxelCell{}, VoxelCell{.x = 1}}));

            EXPECT_TRUE(voxels.contains(VoxelPosition{.x = 1}));
            EXPECT_TRUE(
                voxels.contains(VoxelPosition{.x = kCubeSide}));
        }

    
        TEST(VoxelCubeTest, EverySide_NamesEachOfTheFourJustOnce)
        {
            EXPECT_EQ(kEverySide.size(), kFaceSides);

            const std::set<Side> sides(
                kEverySide.begin(), kEverySide.end());

            EXPECT_EQ(sides.size(), kFaceSides);

            for (const auto edge : kEveryFaceEdge)
            {
                EXPECT_THAT(
                    kEverySide, testing::Contains(edge.side));
            }
        }

        TEST(VoxelCubeTest, Facing_TurnsEverySideIntoTheOneOpposite)
        {
            EXPECT_EQ(facing(Side::Top), Side::Bottom);
            EXPECT_EQ(facing(Side::Bottom), Side::Top);
            EXPECT_EQ(facing(Side::Left), Side::Right);
            EXPECT_EQ(facing(Side::Right), Side::Left);
        }

        TEST(VoxelCubeTest, Facing_ComesBackWhereItStartedTakenTwice)
        {
            for (const auto edge : kEveryFaceEdge)
            {
                EXPECT_NE(facing(edge), edge);
                EXPECT_EQ(facing(facing(edge)), edge);
            }
        }

        TEST(VoxelCubeTest, Facing_LeavesTheKindOfAnEdgeAlone)
        {
            for (const auto edge : kEveryFaceEdge)
            {
                EXPECT_EQ(facing(edge).edge, edge.edge);
            }
        }

    
        TEST(VoxelCubeTest, WithBlockAt_PutsAWholeCubeIn)
        {
            const auto updatedCells = withBlockAt({}, VoxelPosition{.x = 5});

            EXPECT_EQ(updatedCells.size(), kCubeVoxels);

            for (const auto &[position, material] : updatedCells)
            {
                EXPECT_EQ(
                    cubeCornerOf(position),
                    cubeCornerOf(VoxelPosition{.x = 5}));
            }
        }

        TEST(VoxelCubeTest, WithBlockAt_LeavesTheRestOfThePileAlone)
        {
            const auto beforeCells = voxelsOf(
                {VoxelCell{.x = 100}, VoxelCell{.x = 101}});
            const auto updatedCells = withBlockAt(beforeCells, VoxelPosition{});

            EXPECT_EQ(updatedCells.size(), beforeCells.size() + kCubeVoxels);

            for (const auto &[position, material] : beforeCells)
            {
                EXPECT_TRUE(updatedCells.contains(position));
            }
        }

        TEST(VoxelCubeTest, WithBlockAt_PutsACubeAlreadyStandingOnce)
        {
            const auto once = withBlockAt({}, VoxelPosition{});
            const auto twice = withBlockAt(once, VoxelPosition{.x = 1});

            EXPECT_EQ(twice.size(), once.size());
        }

        TEST(VoxelCubeTest, WithoutBlockAt_TakesTheWholeCubeOut)
        {
            const auto voxels = withBlockAt(
                withBlockAt({}, VoxelPosition{}),
                VoxelPosition{.x = kCubeSide});
            const auto updatedCells =
                withoutBlockAt(voxels, VoxelPosition{.x = 1});

            EXPECT_EQ(updatedCells.size(), kCubeVoxels);

            for (const auto &[position, material] : updatedCells)
            {
                EXPECT_NE(
                    cubeCornerOf(position), cubeCornerOf(VoxelPosition{}));
            }
        }

        TEST(VoxelCubeTest, WithoutBlockAt_TakesNothingOutOfAnEmptyPile)
        {
            EXPECT_TRUE(withoutBlockAt({}, VoxelPosition{}).empty());
        }

        TEST(VoxelCubeTest, WithBlockAt_AndWithoutComeBackToWhereItWas)
        {
            const auto beforeCells = expandCubesToVoxels(
                voxelsOf({VoxelCell{}, VoxelCell{.x = 1}}));
            const auto updatedCells = withoutBlockAt(
                withBlockAt(beforeCells, VoxelPosition{.x = 40}),
                VoxelPosition{.x = 40});

            EXPECT_EQ(beforeCells, updatedCells);
        }

        }

}

namespace antwika::voxel
{

    namespace
    {
    TEST(VoxelCubeTest, CubeVoxels_FillsThePlaceForASolidCube)
    {
    
        const auto voxels = cubeVoxels(
            VoxelPosition{}, Kind::Water, VoxelPosition{.x = 1});

        EXPECT_EQ(voxels.size(), kCubeVoxels);

        for (const auto &[position, material] : voxels)
        {
            EXPECT_EQ(material.kind, Kind::Water);
        }
    }

    TEST(VoxelCubeTest, CubeVoxels_LeavesARampOpenAboveItsLowSide)
    {
    
        const auto voxels =
            cubeVoxels(VoxelPosition{}, Kind::Ramp, VoxelPosition{.x = 1});

        EXPECT_EQ(voxels.size(), 6U);

        for (const auto &[position, material] : voxels)
        {
            EXPECT_FALSE(position.x == 0 && position.y == 1);
        }

        for (const auto &[position, material] : voxels)
        {
            EXPECT_EQ(material.kind, Kind::Ramp);
        }
    }

    TEST(VoxelCubeTest, CubeVoxels_TurnsARampAboutWithItsClimb)
    {
    
        const auto voxels =
            cubeVoxels(VoxelPosition{}, Kind::Ramp, VoxelPosition{.z = -1});

        for (const auto &[position, material] : voxels)
        {
            EXPECT_FALSE(position.z == 1 && position.y == 1);
        }
    }

    TEST(VoxelCubeTest, RampDirectionFor_RisesTowardsTheCubeBesideIt)
    {
    
        const auto standing = withBlockAt(
            Voxels{},
            VoxelPosition{.x = kCubeSide, .y = 0, .z = 0});
        const auto climb = rampDirectionFor(standing, VoxelPosition{});

        EXPECT_EQ(climb.x, 1);
        EXPECT_EQ(climb.z, 0);
    }

    TEST(VoxelCubeTest, WithBlockAt_LaysOneKindOverAnother)
    {
    
        const auto solid =
            withBlockAt(Voxels{}, VoxelPosition{});
        const auto watery = withBlockAt(solid, VoxelPosition{}, Kind::Water);

        EXPECT_EQ(watery.size(), kCubeVoxels);

        for (const auto &[position, material] : watery)
        {
            EXPECT_EQ(material.kind, Kind::Water);
        }
    }

    TEST(VoxelCubeTest, WithBlockAt_LeavesTheCubesAboutItAlone)
    {
    
        const auto standing = withBlockAt(
            Voxels{},
            VoxelPosition{.x = kCubeSide, .y = 0, .z = 0});
        const auto voxels =
            withBlockAt(standing, VoxelPosition{}, Kind::Ramp);

        for (const auto &[position, material] : standing)
        {
            EXPECT_TRUE(voxels.contains(position));
        }
    }
    }

}

TEST(VoxelCubeTest, WithBlockAt_ShapesARampTheWayItIsTold)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::inferredRampDirection;
    using antwika::voxel::stepVectorFor;
    using antwika::voxel::withBlockAt;

    const auto voxels = withBlockAt(
        {}, antwika::voxel::VoxelPosition{}, Kind::Ramp, Facing::North);

    ASSERT_FALSE(voxels.empty());

    for (const auto &[position, material] : voxels)
    {
        EXPECT_EQ(material.facing, Facing::North);
        EXPECT_EQ(
            inferredRampDirection(voxels, position),
            stepVectorFor(Facing::North));
    }
}

TEST(VoxelCubeTest, WithBlockAt_TellsASolidBlockNothing)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::withBlockAt;

    for (const auto &[position, material] :
         withBlockAt(
             {},
             antwika::voxel::VoxelPosition{},
             Kind::Normal,
             Facing::North))
    {
        EXPECT_EQ(material.facing, Facing::Any);
    }
}

TEST(VoxelCubeTest, RampDirectionFor_IsDrawnByGroundAndNotByAnotherRamp)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::rampDirectionFor;
    using antwika::voxel::Kind;

    antwika::voxel::Voxels voxels;

    for (const auto &[position, material] :
         cubeVoxels(
             antwika::voxel::VoxelPosition{.x = 2},
             Kind::Ramp,
             antwika::voxel::VoxelPosition{.x = -1}))
    {
        voxels[position] = material;
    }

    for (const auto &[position, material] :
         cubeVoxels(
             antwika::voxel::VoxelPosition{.z = -2},
             Kind::Normal,
             antwika::voxel::VoxelPosition{}))
    {
        voxels[position] = material;
    }

    EXPECT_EQ(
        rampDirectionFor(voxels, antwika::voxel::VoxelPosition{}),
        antwika::voxel::VoxelPosition{.z = -1});
}

TEST(VoxelCubeTest, WithRampsRebuilt_TurnsARampToGroundLaidBesideIt)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::inferredRampDirection;
    using antwika::voxel::stepVectorFor;
    using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;
using antwika::voxel::voxelsOf;
using antwika::voxel::Voxels;
    using antwika::voxel::withBlockAt;
    using antwika::voxel::withoutBlockAt;
    using antwika::voxel::withRampsRebuilt;

    const VoxelPosition rampPosition{};
    const VoxelPosition southPosition{.z = kCubeSide};
    const VoxelPosition westPosition{.x = -kCubeSide};

    auto pile = withBlockAt({}, southPosition, Kind::Normal);
    pile = withRampsRebuilt(
        withBlockAt(pile, rampPosition, Kind::Ramp), rampPosition);

    for (const auto &[position, material] : pile)
    {
        if (material.kind == Kind::Ramp)
        {
            EXPECT_EQ(
                inferredRampDirection(pile, position),
                stepVectorFor(Facing::South));
        }
    }

    pile = withRampsRebuilt(withoutBlockAt(pile, southPosition), southPosition);
    pile = withRampsRebuilt(
        withBlockAt(pile, westPosition, Kind::Normal), westPosition);

    auto stoodCount = 0;

    for (const auto &[position, material] : pile)
    {
        if (material.kind == Kind::Ramp)
        {
            EXPECT_EQ(
                inferredRampDirection(pile, position),
                stepVectorFor(Facing::West));
            ++stoodCount;
        }
    }

    EXPECT_GT(stoodCount, 0);
}

TEST(VoxelCubeTest, WithRampsRebuilt_LeavesARampThatWasToldItsWay)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::inferredRampDirection;
    using antwika::voxel::stepVectorFor;
    using antwika::voxel::VoxelPosition;
    using antwika::voxel::withBlockAt;
    using antwika::voxel::withRampsRebuilt;

    const VoxelPosition rampPosition{};
    const VoxelPosition westPosition{.x = -kCubeSide};

    auto pile = withBlockAt({}, rampPosition, Kind::Ramp, Facing::North);
    pile = withRampsRebuilt(
        withBlockAt(pile, westPosition, Kind::Normal), westPosition);

    for (const auto &[position, material] : pile)
    {
        if (material.kind == Kind::Ramp)
        {
            EXPECT_EQ(
                inferredRampDirection(pile, position),
                stepVectorFor(Facing::North));
        }
    }
}

TEST(VoxelCubeTest, WithRampsRebuilt_LeavesAPileItWouldLayTheSameWay)
{
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::VoxelPosition;
    using antwika::voxel::withBlockAt;
    using antwika::voxel::withRampsRebuilt;

    const VoxelPosition rampPosition{};
    const VoxelPosition westPosition{.x = -kCubeSide};

    auto pile = withBlockAt({}, westPosition, Kind::Normal);
    pile = withRampsRebuilt(
        withBlockAt(pile, rampPosition, Kind::Ramp), rampPosition);

    const auto rebuiltCells = withRampsRebuilt(pile, westPosition);

    EXPECT_EQ(rebuiltCells, pile);
}

TEST(VoxelCubeTest, RampDirectionFor_LeavesAFlightAWayToBeWalkedOntoFrom)
{
    using antwika::voxel::rampDirectionFor;
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::stepVectorFor;
    using antwika::voxel::VoxelPosition;
    using antwika::voxel::withBlockAt;

    auto pile = withBlockAt(
        {}, VoxelPosition{.x = kCubeSide}, Kind::Normal);
    pile = withBlockAt(
        pile, VoxelPosition{.x = -kCubeSide}, Kind::Normal);
    pile = withBlockAt(
        pile, VoxelPosition{.z = -kCubeSide}, Kind::Normal);

    EXPECT_EQ(
        rampDirectionFor(pile, VoxelPosition{}),
        stepVectorFor(Facing::North));
}

TEST(VoxelCubeTest, RampDirectionFor_TakesAnyGroundWhereNoWayInStandsClear)
{
    using antwika::voxel::rampDirectionFor;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::VoxelPosition;
    using antwika::voxel::withBlockAt;

    auto pile = withBlockAt(
        {}, VoxelPosition{.x = kCubeSide}, Kind::Normal);

    for (const auto step : {VoxelPosition{.x = -kCubeSide},
                            VoxelPosition{.z = kCubeSide},
                            VoxelPosition{.z = -kCubeSide}})
    {
        pile = withBlockAt(pile, step, Kind::Normal);
    }

    const auto climb = rampDirectionFor(pile, VoxelPosition{});

    EXPECT_TRUE(climb.x != 0 || climb.z != 0);
}
