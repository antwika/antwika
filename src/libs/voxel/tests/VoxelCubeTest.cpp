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
            EXPECT_EQ(cubeCells(VoxelCell{}).size(), kCubeVoxels);
        }

        TEST(VoxelCubeTest, CubeCells_HoldsNoVoxelTwice)
        {
            const auto cells = cubeCells(VoxelCell{.x = 4, .y = -2});
            const std::set<VoxelCell> apartCells(
                cells.begin(), cells.end());

            EXPECT_EQ(apartCells.size(), cells.size());
        }

        TEST(VoxelCubeTest, CubeCells_ReachesOneSideFromItsCorner)
        {
            const VoxelCell cornerCell{.x = 6, .y = -4, .z = 2};

            for (const auto cell : cubeCells(cornerCell))
            {
                EXPECT_GE(cell.x, cornerCell.x);
                EXPECT_GE(cell.y, cornerCell.y);
                EXPECT_GE(cell.z, cornerCell.z);
                EXPECT_LT(cell.x, cornerCell.x + kCubeSide);
                EXPECT_LT(cell.y, cornerCell.y + kCubeSide);
                EXPECT_LT(cell.z, cornerCell.z + kCubeSide);
            }
        }

        TEST(VoxelCubeTest, CubeCornerOf_LaysTheVoxelsOfACubeInThatCube)
        {
            const VoxelCell cornerCell{.x = -6, .y = 2, .z = 4};

            for (const auto cell : cubeCells(cornerCell))
            {
                EXPECT_EQ(cubeCornerOf(cell), cornerCell);
            }
        }

        TEST(VoxelCubeTest, CubeCornerOf_NamesACubeByItsLowestCorner)
        {
            EXPECT_EQ(
                cubeCornerOf(VoxelCell{.x = 3, .y = 2, .z = 1}),
                (VoxelCell{.x = 2, .y = 2, .z = 0}));
        }

        TEST(VoxelCubeTest, CubeCornerOf_LaysAVoxelBelowNoughtInTheCubeBelowIt)
        {
            EXPECT_EQ(
                cubeCornerOf(VoxelCell{.x = -1, .y = -2, .z = -3}),
                (VoxelCell{.x = -2, .y = -2, .z = -4}));
        }

        TEST(VoxelCubeTest, CubeCornerOf_LeavesNoVoxelBetweenNeighbouringCubes)
        {
            const auto hereCells =
                cubeCells(cubeCornerOf(VoxelCell{.x = 0}));
            const auto thereCells =
                cubeCells(cubeCornerOf(VoxelCell{.x = kCubeSide}));
            std::set<VoxelCell> bothCells(hereCells.begin(), hereCells.end());

            bothCells.insert(thereCells.begin(), thereCells.end());

            EXPECT_EQ(bothCells.size(), kCubeVoxels * 2);
            EXPECT_TRUE(bothCells.contains(VoxelCell{.x = 1}));
            EXPECT_TRUE(bothCells.contains(VoxelCell{.x = kCubeSide}));
        }

        TEST(VoxelCubeTest, ExpandCubesToVoxels_GivesAWholeCubePerCell)
        {
            const std::vector<VoxelCell> cells{
                VoxelCell{}, VoxelCell{.x = 1}};

            EXPECT_EQ(
                expandCubesToVoxels(cells).size(),
                cells.size() * kCubeVoxels);
        }

        TEST(VoxelCubeTest, ExpandCubesToVoxels_LetsNoTwoBlocksOverlap)
        {
            const std::vector<VoxelCell> cells{
                VoxelCell{},
                VoxelCell{.x = 1},
                VoxelCell{.y = 1},
                VoxelCell{.x = -1, .z = 2}};
            const auto voxels = expandCubesToVoxels(cells);
            const std::set<VoxelCell> apartCells(
                voxels.begin(), voxels.end());

            EXPECT_EQ(apartCells.size(), voxels.size());
        }

        TEST(VoxelCubeTest, ExpandCubesToVoxels_LaysEveryVoxelOfACellInOneCube)
        {
            const std::vector<VoxelCell> cells{
                VoxelCell{.x = 2, .y = -1}};
            const auto voxels = expandCubesToVoxels(cells);
            std::set<VoxelCell> cubeCells;

            for (const auto voxel : voxels)
            {
                cubeCells.insert(cubeCornerOf(voxel));
            }

            EXPECT_EQ(cubeCells.size(), 1U);
        }

        TEST(
    VoxelCubeTest,
    ExpandCubesToVoxels_KeepsNeighbouringCellsNeighbours)
        {
            const auto voxels = expandCubesToVoxels(
                {VoxelCell{}, VoxelCell{.x = 1}});
            const std::set<VoxelCell> cells(
                voxels.begin(), voxels.end());

            EXPECT_TRUE(cells.contains(VoxelCell{.x = 1}));
            EXPECT_TRUE(
                cells.contains(VoxelCell{.x = kCubeSide}));
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
            const auto updatedCells = withBlockAt({}, VoxelCell{.x = 5});

            EXPECT_EQ(updatedCells.size(), kCubeVoxels);

            const std::set<VoxelCell> cells(
                updatedCells.begin(), updatedCells.end());

            EXPECT_EQ(cells.size(), kCubeVoxels);

            for (const auto voxel : updatedCells)
            {
                EXPECT_EQ(
                    cubeCornerOf(voxel),
                    cubeCornerOf(VoxelCell{.x = 5}));
            }
        }

        TEST(VoxelCubeTest, WithBlockAt_LeavesTheRestOfThePileAlone)
        {
            const std::vector<VoxelCell> beforeCells{
                VoxelCell{.x = 100}, VoxelCell{.x = 101}};
            const auto updatedCells = withBlockAt(beforeCells, VoxelCell{});

            EXPECT_EQ(updatedCells.size(), beforeCells.size() + kCubeVoxels);

            for (const auto voxel : beforeCells)
            {
                EXPECT_NE(
                    std::ranges::find(updatedCells, voxel), updatedCells.end());
            }
        }

        TEST(VoxelCubeTest, WithBlockAt_PutsACubeAlreadyStandingOnce)
        {
            const auto once = withBlockAt({}, VoxelCell{});
            const auto twice = withBlockAt(once, VoxelCell{.x = 1});

            EXPECT_EQ(twice.size(), once.size());
        }

        TEST(VoxelCubeTest, WithoutBlockAt_TakesTheWholeCubeOut)
        {
            const auto cells = withBlockAt(
                withBlockAt({}, VoxelCell{}),
                VoxelCell{.x = kCubeSide});
            const auto updatedCells =
                withoutBlockAt(cells, VoxelCell{.x = 1});

            EXPECT_EQ(updatedCells.size(), kCubeVoxels);

            for (const auto voxel : updatedCells)
            {
                EXPECT_NE(
                    cubeCornerOf(voxel), cubeCornerOf(VoxelCell{}));
            }
        }

        TEST(VoxelCubeTest, WithoutBlockAt_TakesNothingOutOfAnEmptyPile)
        {
            EXPECT_TRUE(withoutBlockAt({}, VoxelCell{}).empty());
        }

        TEST(VoxelCubeTest, WithBlockAt_AndWithoutComeBackToWhereItWas)
        {
            const auto beforeCells = expandCubesToVoxels(
                {VoxelCell{}, VoxelCell{.x = 1}});
            const auto updatedCells = withoutBlockAt(
                withBlockAt(beforeCells, VoxelCell{.x = 40}),
                VoxelCell{.x = 40});
            const std::set<VoxelCell> sortedBeforeCells(
                beforeCells.begin(), beforeCells.end());
            const std::set<VoxelCell> sortedAfterCells(
                updatedCells.begin(), updatedCells.end());

            EXPECT_EQ(sortedBeforeCells, sortedAfterCells);
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
            VoxelCell{}, Kind::Water, VoxelCell{.x = 1});

        EXPECT_EQ(voxels.size(), kCubeVoxels);

        for (const auto cell : voxels)
        {
            EXPECT_EQ(cell.kind, Kind::Water);
        }
    }

    TEST(VoxelCubeTest, CubeVoxels_LeavesARampOpenAboveItsLowSide)
    {
    
        const auto voxels =
            cubeVoxels(VoxelCell{}, Kind::Ramp, VoxelCell{.x = 1});

        EXPECT_EQ(voxels.size(), 6U);

        for (const auto cell : voxels)
        {
            EXPECT_FALSE(cell.x == 0 && cell.y == 1);
        }

        for (const auto cell : voxels)
        {
            EXPECT_EQ(cell.kind, Kind::Ramp);
        }
    }

    TEST(VoxelCubeTest, CubeVoxels_TurnsARampAboutWithItsClimb)
    {
    
        const auto voxels =
            cubeVoxels(VoxelCell{}, Kind::Ramp, VoxelCell{.z = -1});

        for (const auto cell : voxels)
        {
            EXPECT_FALSE(cell.z == 1 && cell.y == 1);
        }
    }

    TEST(VoxelCubeTest, RampDirectionFor_RisesTowardsTheCubeBesideIt)
    {
    
        const auto standing = withBlockAt(
            std::vector<VoxelCell>{},
            VoxelCell{.x = kCubeSide, .y = 0, .z = 0});
        const auto climb = rampDirectionFor(standing, VoxelCell{});

        EXPECT_EQ(climb.x, 1);
        EXPECT_EQ(climb.z, 0);
    }

    TEST(VoxelCubeTest, WithBlockAt_LaysOneKindOverAnother)
    {
    
        const auto solid =
            withBlockAt(std::vector<VoxelCell>{}, VoxelCell{});
        const auto watery = withBlockAt(solid, VoxelCell{}, Kind::Water);

        EXPECT_EQ(watery.size(), kCubeVoxels);

        for (const auto cell : watery)
        {
            EXPECT_EQ(cell.kind, Kind::Water);
        }
    }

    TEST(VoxelCubeTest, WithBlockAt_LeavesTheCubesAboutItAlone)
    {
    
        const auto standing = withBlockAt(
            std::vector<VoxelCell>{},
            VoxelCell{.x = kCubeSide, .y = 0, .z = 0});
        const auto cells =
            withBlockAt(standing, VoxelCell{}, Kind::Ramp);

        for (const auto cell : standing)
        {
            EXPECT_NE(
                std::find(cells.begin(), cells.end(), cell),
                cells.end());
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

    const auto cells = withBlockAt(
        {}, antwika::voxel::VoxelCell{}, Kind::Ramp, Facing::North);

    ASSERT_FALSE(cells.empty());

    for (const auto cell : cells)
    {
        EXPECT_EQ(cell.facing, Facing::North);
        EXPECT_EQ(
            inferredRampDirection(cells, cell),
            stepVectorFor(Facing::North));
    }
}

TEST(VoxelCubeTest, WithBlockAt_TellsASolidBlockNothing)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::withBlockAt;

    for (const auto cell :
         withBlockAt(
             {},
             antwika::voxel::VoxelCell{},
             Kind::Normal,
             Facing::North))
    {
        EXPECT_EQ(cell.facing, Facing::Any);
    }
}

TEST(VoxelCubeTest, RampDirectionFor_IsDrawnByGroundAndNotByAnotherRamp)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::rampDirectionFor;
    using antwika::voxel::Kind;

    std::vector<antwika::voxel::VoxelCell> cells;

    for (const auto cell :
         cubeVoxels(
             antwika::voxel::VoxelCell{.x = 2},
             Kind::Ramp,
             antwika::voxel::VoxelCell{.x = -1}))
    {
        cells.push_back(cell);
    }

    for (const auto cell :
         cubeVoxels(
             antwika::voxel::VoxelCell{.z = -2},
             Kind::Normal,
             antwika::voxel::VoxelCell{}))
    {
        cells.push_back(cell);
    }

    EXPECT_EQ(
        rampDirectionFor(cells, antwika::voxel::VoxelCell{}),
        antwika::voxel::VoxelCell{.z = -1});
}

TEST(VoxelCubeTest, WithRampsRebuilt_TurnsARampToGroundLaidBesideIt)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::inferredRampDirection;
    using antwika::voxel::stepVectorFor;
    using antwika::voxel::VoxelCell;
    using antwika::voxel::withBlockAt;
    using antwika::voxel::withoutBlockAt;
    using antwika::voxel::withRampsRebuilt;

    const VoxelCell rampCell{};
    const VoxelCell southCell{.z = kCubeSide};
    const VoxelCell westCell{.x = -kCubeSide};

    auto pile = withBlockAt({}, southCell, Kind::Normal);
    pile = withRampsRebuilt(
        withBlockAt(pile, rampCell, Kind::Ramp), rampCell);

    for (const auto cell : pile)
    {
        if (cell.kind == Kind::Ramp)
        {
            EXPECT_EQ(
                inferredRampDirection(pile, cell),
                stepVectorFor(Facing::South));
        }
    }

    pile = withRampsRebuilt(withoutBlockAt(pile, southCell), southCell);
    pile = withRampsRebuilt(
        withBlockAt(pile, westCell, Kind::Normal), westCell);

    auto stoodCount = 0;

    for (const auto cell : pile)
    {
        if (cell.kind == Kind::Ramp)
        {
            EXPECT_EQ(
                inferredRampDirection(pile, cell),
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
    using antwika::voxel::VoxelCell;
    using antwika::voxel::withBlockAt;
    using antwika::voxel::withRampsRebuilt;

    const VoxelCell rampCell{};
    const VoxelCell westCell{.x = -kCubeSide};

    auto pile = withBlockAt({}, rampCell, Kind::Ramp, Facing::North);
    pile = withRampsRebuilt(
        withBlockAt(pile, westCell, Kind::Normal), westCell);

    for (const auto cell : pile)
    {
        if (cell.kind == Kind::Ramp)
        {
            EXPECT_EQ(
                inferredRampDirection(pile, cell),
                stepVectorFor(Facing::North));
        }
    }
}

TEST(VoxelCubeTest, WithRampsRebuilt_LeavesAPileItWouldLayTheSameWay)
{
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::VoxelCell;
    using antwika::voxel::withBlockAt;
    using antwika::voxel::withRampsRebuilt;

    const VoxelCell rampCell{};
    const VoxelCell westCell{.x = -kCubeSide};

    auto pile = withBlockAt({}, westCell, Kind::Normal);
    pile = withRampsRebuilt(
        withBlockAt(pile, rampCell, Kind::Ramp), rampCell);

    const std::set<VoxelCell> beforeCells(pile.begin(), pile.end());
    const auto rebuiltCells = withRampsRebuilt(pile, westCell);

    EXPECT_EQ(rebuiltCells.size(), pile.size());
    EXPECT_EQ(std::set<VoxelCell>(rebuiltCells.begin(), rebuiltCells.end()),
        beforeCells);
}

TEST(VoxelCubeTest, RampDirectionFor_LeavesAFlightAWayToBeWalkedOntoFrom)
{
    using antwika::voxel::rampDirectionFor;
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::stepVectorFor;
    using antwika::voxel::VoxelCell;
    using antwika::voxel::withBlockAt;

    auto pile = withBlockAt(
        {}, VoxelCell{.x = kCubeSide}, Kind::Normal);
    pile = withBlockAt(
        pile, VoxelCell{.x = -kCubeSide}, Kind::Normal);
    pile = withBlockAt(
        pile, VoxelCell{.z = -kCubeSide}, Kind::Normal);

    EXPECT_EQ(
        rampDirectionFor(pile, VoxelCell{}),
        stepVectorFor(Facing::North));
}

TEST(VoxelCubeTest, RampDirectionFor_TakesAnyGroundWhereNoWayInStandsClear)
{
    using antwika::voxel::rampDirectionFor;
    using antwika::voxel::Kind;
    using antwika::voxel::kCubeSide;
    using antwika::voxel::VoxelCell;
    using antwika::voxel::withBlockAt;

    auto pile = withBlockAt(
        {}, VoxelCell{.x = kCubeSide}, Kind::Normal);

    for (const auto step : {VoxelCell{.x = -kCubeSide},
                            VoxelCell{.z = kCubeSide},
                            VoxelCell{.z = -kCubeSide}})
    {
        pile = withBlockAt(pile, step, Kind::Normal);
    }

    const auto climb = rampDirectionFor(pile, VoxelCell{});

    EXPECT_TRUE(climb.x != 0 || climb.z != 0);
}
