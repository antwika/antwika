#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <vector>

#include <antwika/solver/VoxelWeave.hpp>

using antwika::tilemap::Atlas;
using antwika::voxel::Voxels;
using antwika::solver::crossLevelSeams;
using antwika::solver::sameLevelSeams;
using antwika::voxel::expandCubesToVoxels;
using antwika::voxel::voxelsOf;
using antwika::solver::CornerSeams;
using antwika::voxel::EdgeKind;
using antwika::voxel::facing;
using antwika::voxelmap::visibleFacesOf;
using antwika::voxelmap::faceNormal;
using antwika::solver::faceAdjacency;
using antwika::tilemap::kEveryTileEdge;
using antwika::solver::kTileDomainSize;
using antwika::voxelmap::demoCells;
using antwika::tilemap::Tile;
using antwika::tile::TileRules;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelMaterial;
using antwika::voxel::VoxelPosition;
using antwika::voxel::Side;
using antwika::tilemap::TileEdge;
using antwika::solver::solveTiles;
using antwika::solver::satisfiedSeams;
using antwika::solver::isCornerSeam;
using antwika::solver::tileFromIndex;
using antwika::solver::WeaveGap;
using antwika::solver::missingRules;
using antwika::solver::SolveFailure;
using antwika::solver::tileToIndex;
using antwika::solver::solvedTiles;

namespace
{
    [[nodiscard]] bool liesFlat(
        const std::vector<antwika::voxelmap::FaceRef> &faces,
        const antwika::solver::FaceSeam &seam)
    {
        return faces[seam.faceA].side == faces[seam.faceB].side;
    }

    [[nodiscard]] TileRules allowingAmong(
        const std::vector<Tile> &tiles)
    {
        TileRules rules;

        for (const auto one : tiles)
        {
            for (const auto edge : kEveryTileEdge)
            {
                rules.setAllowsBoundary(one, edge, true);

                for (const auto other : tiles)
                {
                    rules.allow(one, edge, other);
                }
            }
        }

        return rules;
    }

    [[nodiscard]] TileRules allowingAFew()
    {
        return allowingAmong(
            {Tile{.atlas = Atlas::Wall, .index = 0},
             Tile{.atlas = Atlas::Wall, .index = 1},
             Tile{.atlas = Atlas::Floor, .index = 0},
             Tile{.atlas = Atlas::Floor, .index = 1}});
    }
}

TEST(VoxelWeaveTest, TileToIndex_TellsTheTwoAtlasesApart)
{
    constexpr Tile flatTile{.atlas = Atlas::Floor, .index = 5};
    constexpr Tile uprightTile{.atlas = Atlas::Wall, .index = 5};

    EXPECT_NE(tileToIndex(flatTile), tileToIndex(uprightTile));
    EXPECT_LT(tileToIndex(flatTile), kTileDomainSize);
    EXPECT_LT(tileToIndex(uprightTile), kTileDomainSize);
}

TEST(VoxelWeaveTest, TileFromIndex_UndoesTileToIndex)
{
    for (std::size_t value = 0; value < kTileDomainSize; ++value)
    {
        EXPECT_EQ(tileToIndex(tileFromIndex(value)), value);
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_JoinsTheSidesOfALoneVoxelAtItsEdges)
{
    const auto faces = visibleFacesOf(voxelsOf({VoxelCell{}}));
    const auto seams = faceAdjacency(faces);

    EXPECT_EQ(seams.size(), 12U);

    for (const auto &seam : seams)
    {
        EXPECT_FALSE(liesFlat(faces, seam));
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_JoinsTheTopsOfTwoVoxelsSideBySide)
{
    const auto pairCells = voxelsOf({VoxelCell{},
        VoxelCell{.position = {.x = 1}}});
    const auto faces = visibleFacesOf(pairCells);
    auto joinedCount = 0;

    for (const auto &seam : faceAdjacency(faces))
    {
        if (liesFlat(faces, seam)
            && faceNormal(faces[seam.faceA].side).y > 0.0F)
        {
            ++joinedCount;
        }
    }

    EXPECT_EQ(joinedCount, 1);
}

TEST(VoxelWeaveTest, FaceAdjacency_GivesEachPairOfFacesJustOnce)
{
    const auto seams = faceAdjacency(visibleFacesOf(demoCells()));
    std::set<std::pair<std::size_t, std::size_t>> seenPairs;

    for (const auto &seam : seams)
    {
        const auto lowFace = std::min(seam.faceA, seam.faceB);
        const auto highFace = std::max(seam.faceA, seam.faceB);

        EXPECT_TRUE(seenPairs.insert({lowFace, highFace}).second);
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_FacesTheEdgesOfAFlatSeamAtEachOther)
{
    const auto faces = visibleFacesOf(demoCells());

    for (const auto &seam : faceAdjacency(faces))
    {
        if (!liesFlat(faces, seam))
        {
            continue;
        }

        EXPECT_EQ(
            seam.edgeB.side,
            antwika::voxel::facing(seam.edgeA.side));
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_GivesBothEndsOfASeamTheOneKind)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));

    for (const auto &seam : faceAdjacency(faces))
    {
        EXPECT_EQ(seam.edgeA.edge, seam.edgeB.edge);
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_JoinsATopFaceToTheSideBesideIt)
{
    const auto faces = visibleFacesOf(voxelsOf({VoxelCell{}}));
    auto joinedCount = 0;

    for (const auto &seam : faceAdjacency(faces))
    {
        const auto hereFlat = faceNormal(faces[seam.faceA].side).y != 0.0F;
        const auto thereFlat =
            faceNormal(faces[seam.faceB].side).y != 0.0F;

        joinedCount += hereFlat != thereFlat ? 1 : 0;
    }

    EXPECT_EQ(joinedCount, 8);
}

TEST(VoxelWeaveTest, FaceAdjacency_CallsAFlatSeamInsideOneCubeInterior)
{
    const auto pairCells = voxelsOf({VoxelCell{},
        VoxelCell{.position = {.x = 1}}});
    const auto faces = visibleFacesOf(pairCells);

    for (const auto &seam : faceAdjacency(faces))
    {
        if (liesFlat(faces, seam))
        {
            EXPECT_EQ(seam.edgeA.edge, EdgeKind::Interior);
        }
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_CallsAFlatSeamBetweenTwoCubesBoundary)
{
    const auto pairCells = voxelsOf({
        VoxelCell{.position = {.x = 1}}, VoxelCell{.position = {.x = 2}}});
    const auto faces = visibleFacesOf(pairCells);

    for (const auto &seam : faceAdjacency(faces))
    {
        if (liesFlat(faces, seam))
        {
            EXPECT_EQ(seam.edgeA.edge, EdgeKind::Boundary);
        }
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_CallsACornerOnACubesOwnEdgeBoundary)
{
    const auto voxels = expandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto faces = visibleFacesOf(voxels);
    auto outward = 0;

    for (const auto &seam : faceAdjacency(faces))
    {
        if (!liesFlat(faces, seam)
            && seam.edgeA.edge == EdgeKind::Boundary)
        {
            ++outward;
        }
    }

    EXPECT_GT(outward, 0);
}

TEST(VoxelWeaveTest, SolvedTiles_SolvesNothingWhileNothingIsAllowed)
{
    EXPECT_FALSE(
        solvedTiles(visibleFacesOf(demoCells()), TileRules{})
            .has_value());
}

TEST(VoxelWeaveTest, SolvedTiles_GivesOneTilePerVisibleFace)
{
    const auto faces = visibleFacesOf(demoCells());
    const auto woven = solvedTiles(faces, allowingAFew());

    ASSERT_TRUE(woven.has_value());
    EXPECT_EQ(woven->size(), faces.size());
}

TEST(VoxelWeaveTest, SolvedTiles_DrawsEachFaceFromItsOwnAtlas)
{
    const auto faces = visibleFacesOf(demoCells());
    const auto woven = solvedTiles(faces, allowingAFew());

    ASSERT_TRUE(woven.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        const auto flatTile = faceNormal(faces[index].side).y != 0.0F;

        EXPECT_EQ(
            (*woven)[index].atlas,
            flatTile ? Atlas::Floor : Atlas::Wall);
    }
}

TEST(VoxelWeaveTest, SolvedTiles_KeepsEveryRuleItWasGiven)
{
    constexpr Tile flatTile{.atlas = Atlas::Floor, .index = 0};
    constexpr Tile uprightTile{.atlas = Atlas::Wall, .index = 0};
    const auto rules = allowingAmong({flatTile, uprightTile});
    const auto faces = visibleFacesOf(demoCells());
    const auto woven = solvedTiles(faces, rules);

    ASSERT_TRUE(woven.has_value());

    for (const auto &seam : faceAdjacency(faces))
    {
        const auto hereTile = (*woven)[seam.faceA];
        const auto thereTile = (*woven)[seam.faceB];

        EXPECT_TRUE(rules.allows(hereTile, seam.edgeA, thereTile));
        EXPECT_TRUE(rules.allows(thereTile, seam.edgeB, hereTile));
    }
}

TEST(VoxelWeaveTest, SolvedTiles_TakesOnlyTilesTheRulesLeaveOpen)
{
    constexpr Tile flatTile{.atlas = Atlas::Floor, .index = 3};
    constexpr Tile uprightTile{.atlas = Atlas::Wall, .index = 9};
    const auto faces = visibleFacesOf(demoCells());
    const auto woven = solvedTiles(
        faces,
        allowingAmong({flatTile, uprightTile}));

    ASSERT_TRUE(woven.has_value());

    for (const auto tile : *woven)
    {
        EXPECT_TRUE(tile == flatTile || tile == uprightTile);
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_GivesABlockPileSeamsOfBothKinds)
{
    const auto seams =
        faceAdjacency(visibleFacesOf(expandCubesToVoxels(demoCells())));
    auto inward = 0;
    auto outward = 0;

    for (const auto &seam : seams)
    {
        if (seam.edgeA.edge == EdgeKind::Interior)
        {
            ++inward;
        }
        else
        {
            ++outward;
        }
    }

    EXPECT_GT(inward, 0);
    EXPECT_GT(outward, 0);
}

TEST(VoxelWeaveTest, SolvedTiles_SolvesAWholePileOfBlocks)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto rules = allowingAFew();
    const auto woven = solvedTiles(faces, rules);

    ASSERT_TRUE(woven.has_value());
    EXPECT_EQ(woven->size(), faces.size());

    for (const auto &seam : faceAdjacency(faces))
    {
        EXPECT_TRUE(
            rules.allows(
                (*woven)[seam.faceA],
                seam.edgeA,
                (*woven)[seam.faceB]));
    }
}

TEST(VoxelWeaveTest, SolveTiles_SaysNothingIsWrongWhereASolveIsFound)
{
    const auto solution =
        solveTiles(
            visibleFacesOf(expandCubesToVoxels(demoCells())),
            allowingAFew());

    ASSERT_TRUE(solution.tiles.has_value());
    EXPECT_EQ(solution.troubleFailure, SolveFailure::None);
}

TEST(VoxelWeaveTest, MissingRules_WantsNothingWhereASolveIsFound)
{
    EXPECT_TRUE(
        missingRules(
            visibleFacesOf(expandCubesToVoxels(demoCells())),
            allowingAFew())
            .empty());
}

namespace
{
    constexpr Tile kTopTile{.atlas = Atlas::Floor, .index = 2};

    constexpr Tile kOtherTopTile{.atlas = Atlas::Floor, .index = 7};

    constexpr Tile kSideTile{.atlas = Atlas::Wall, .index = 3};

    [[nodiscard]] TileRules onlyTheTops()
    {
        TileRules rules;

        rules.allow(
            kTopTile,
            TileEdge{.side = Side::Bottom, .edge = EdgeKind::Interior},
            kOtherTopTile);
        rules.allow(
            kOtherTopTile,
            TileEdge{.side = Side::Top, .edge = EdgeKind::Interior},
            kTopTile);

        return rules;
    }
}

TEST(VoxelWeaveTest, SolveTiles_SolvesOneAtlasWhileTheOtherIsLeftAlone)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto solution = solveTiles(faces, onlyTheTops());

    ASSERT_TRUE(solution.tiles.has_value());
    EXPECT_EQ(solution.troubleFailure, SolveFailure::None);
    EXPECT_GT(solution.skippedFaceCount, 0U);
}

TEST(VoxelWeaveTest, SolveTiles_LeavesTheUnspokenAtlasDrawingWhatItDrew)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto beforeTiles = antwika::voxelmap::defaultTiles(faces);
    const auto solution = solveTiles(faces, onlyTheTops());

    ASSERT_TRUE(solution.tiles.has_value());

    auto alone = 0U;

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faceNormal(faces[index].side).y != 0.0F)
        {
            EXPECT_EQ(
                (*solution.tiles)[index].atlas, Atlas::Floor);
            EXPECT_TRUE(
                (*solution.tiles)[index] == kTopTile
                || (*solution.tiles)[index] == kOtherTopTile);
        }
        else
        {
            EXPECT_EQ((*solution.tiles)[index], beforeTiles[index]);
            ++alone;
        }
    }

    EXPECT_EQ(solution.skippedFaceCount, alone);
}

TEST(VoxelWeaveTest, SolveTiles_LetsASilentEdgeStandAgainstAnything)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto rules = onlyTheTops();
    const auto solution = solveTiles(faces, rules);

    ASSERT_TRUE(solution.tiles.has_value());

    for (const auto &seam : faceAdjacency(faces))
    {
        const auto hereTile = (*solution.tiles)[seam.faceA];
        const auto thereTile = (*solution.tiles)[seam.faceB];

        if (!rules.hasNoRuleFor(hereTile, seam.edgeA, thereTile.atlas))
        {
            EXPECT_TRUE(rules.allows(hereTile, seam.edgeA, thereTile));
        }
    }
}

TEST(VoxelWeaveTest, SolveTiles_SaysWhichEdgeIsLeftWithNoPair)
{
    constexpr TileEdge rightInwardEdge{
        .side = Side::Right, .edge = EdgeKind::Interior};
    constexpr TileEdge leftInwardEdge{
        .side = Side::Left, .edge = EdgeKind::Interior};
    TileRules rules;

    rules.allow(kTopTile, rightInwardEdge, kOtherTopTile);
    rules.allow(kTopTile, leftInwardEdge, kTopTile);
    rules.allow(kOtherTopTile, rightInwardEdge, kTopTile);
    rules.allow(kOtherTopTile, leftInwardEdge, kOtherTopTile);
    rules.allow(
        kSideTile,
        TileEdge{.side = Side::Top, .edge = EdgeKind::Interior},
        kSideTile);

    const auto solution =
        solveTiles(visibleFacesOf(expandCubesToVoxels(demoCells())), rules);

    EXPECT_FALSE(solution.tiles.has_value());
    EXPECT_EQ(solution.troubleFailure, SolveFailure::IncompatibleEdge);
}

TEST(VoxelWeaveTest, MissingRules_NamesAnAtlasOnlyWhenNothingHasRules)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto bare = missingRules(faces, TileRules{});

    ASSERT_EQ(bare.size(), 1U);
    EXPECT_EQ(bare.front().troubleFailure, SolveFailure::EmptyDomain);
    EXPECT_TRUE(missingRules(faces, onlyTheTops()).empty());
}

TEST(VoxelWeaveTest, MissingRules_LetsARuleSpeakOnlyOfWhatItNames)
{
    TileRules rules;

    rules.allow(
        kTopTile,
        TileEdge{.side = Side::Right, .edge = EdgeKind::Interior},
        kSideTile);
    rules.allow(
        kSideTile,
        TileEdge{.side = Side::Top, .edge = EdgeKind::Interior},
        kSideTile);

    EXPECT_TRUE(rules.hasNoRuleFor(
        kTopTile,
        TileEdge{.side = Side::Right, .edge = EdgeKind::Interior},
        Atlas::Floor));
    EXPECT_FALSE(rules.hasNoRuleFor(
        kTopTile,
        TileEdge{.side = Side::Right, .edge = EdgeKind::Interior},
        Atlas::Wall));
    EXPECT_TRUE(
        missingRules(
            visibleFacesOf(expandCubesToVoxels(demoCells())), rules)
            .empty());
}

TEST(VoxelWeaveTest, SatisfiedSeams_TiesNothingWhereNothingIsSaid)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto drawnTiles = antwika::voxelmap::defaultTiles(faces);

    EXPECT_TRUE(satisfiedSeams(faces, drawnTiles, TileRules{}).empty());
}

TEST(VoxelWeaveTest, SatisfiedSeams_TiesOnlyWhatBothTilesAgreeTo)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto rules = allowingAFew();
    const auto solution = solveTiles(faces, rules);

    ASSERT_TRUE(solution.tiles.has_value());

    const auto satisfiedSeamSet =
        satisfiedSeams(faces, *solution.tiles, rules);

    EXPECT_FALSE(satisfiedSeamSet.empty());

    for (const auto &seam : satisfiedSeamSet)
    {
        const auto hereTile = (*solution.tiles)[seam.faceA];
        const auto thereTile = (*solution.tiles)[seam.faceB];

        EXPECT_TRUE(rules.allows(hereTile, seam.edgeA, thereTile));
        EXPECT_TRUE(rules.allows(thereTile, seam.edgeB, hereTile));
    }
}

TEST(VoxelWeaveTest, SatisfiedSeams_LeavesASilentSeamUntied)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto rules = onlyTheTops();
    const auto solution = solveTiles(faces, rules);

    ASSERT_TRUE(solution.tiles.has_value());

    const auto satisfiedSeamSet =
        satisfiedSeams(faces, *solution.tiles, rules);

    EXPECT_LT(satisfiedSeamSet.size(), faceAdjacency(faces).size());
}

TEST(VoxelWeaveTest, SameLevelSeams_KeepsOnlySeamsWhollyAtThatLevel)
{
    const auto voxels = expandCubesToVoxels(demoCells());
    const auto faces = visibleFacesOf(voxels);
    const auto seams = faceAdjacency(faces);
    const auto levelSeams = sameLevelSeams(faces, seams, 1);

    EXPECT_FALSE(levelSeams.empty());

    for (const auto &seam : levelSeams)
    {
        EXPECT_EQ(antwika::voxelmap::levelOf(faces[seam.faceA].cell.position),
            1);
        EXPECT_EQ(antwika::voxelmap::levelOf(faces[seam.faceB].cell.position),
            1);
    }
}

TEST(VoxelWeaveTest, SameLevelSeams_LeavesNoSeamOutOfEveryLevelAtOnce)
{
    const auto voxels = expandCubesToVoxels(demoCells());
    const auto faces = visibleFacesOf(voxels);
    const auto seams = faceAdjacency(faces);
    std::size_t gatheredCount = 0;

    for (auto level = antwika::voxelmap::bottomLevel(voxels);
         level <= antwika::voxelmap::topLevel(voxels);
         ++level)
    {
        gatheredCount += sameLevelSeams(faces, seams, level).size();
    }

    EXPECT_LE(gatheredCount, seams.size());
    EXPECT_GT(gatheredCount, 0U);
}

TEST(VoxelWeaveTest, SameLevelSeams_KeepsNothingAtALevelThePileNeverReaches)
{
    const auto voxels = expandCubesToVoxels(demoCells());
    const auto faces = visibleFacesOf(voxels);

    EXPECT_TRUE(sameLevelSeams(faces, faceAdjacency(faces), 99).empty());
}

TEST(VoxelWeaveTest, CrossLevelSeams_KeepsOnlySeamsLeavingThatLevel)
{
    const auto voxels = expandCubesToVoxels(demoCells());
    const auto faces = visibleFacesOf(voxels);
    const auto seams = faceAdjacency(faces);
    const auto crossSeams = crossLevelSeams(faces, seams, 1);

    EXPECT_FALSE(crossSeams.empty());

    for (const auto &seam : crossSeams)
    {
        const auto hereLevel =
        antwika::voxelmap::levelOf(faces[seam.faceA].cell.position);
        const auto thereLevel =
            antwika::voxelmap::levelOf(faces[seam.faceB].cell.position);

        EXPECT_NE(hereLevel, thereLevel);
        EXPECT_TRUE(hereLevel == 1 || thereLevel == 1);
    }
}

TEST(VoxelWeaveTest, CrossLevelSeams_SharesNoSeamWithSameLevelSeams)
{
    const auto voxels = expandCubesToVoxels(demoCells());
    const auto faces = visibleFacesOf(voxels);
    const auto seams = faceAdjacency(faces);
    const auto levelSeams = sameLevelSeams(faces, seams, 1);
    const auto crossSeams = crossLevelSeams(faces, seams, 1);

    for (const auto &one : levelSeams)
    {
        for (const auto &other : crossSeams)
        {
            EXPECT_NE(one, other);
        }
    }
}

TEST(VoxelWeaveTest, CrossLevelSeams_LeavesNoSeamOfALevelUnaccountedFor)
{
    const auto voxels = expandCubesToVoxels(demoCells());
    const auto faces = visibleFacesOf(voxels);
    const auto seams = faceAdjacency(faces);
    const auto touching = std::count_if(
        seams.begin(),
        seams.end(),
        [&](const antwika::solver::FaceSeam &seam)
        {
            return antwika::voxelmap::levelOf(
                faces[seam.faceA].cell.position) == 1
                   || antwika::voxelmap::levelOf(
                       faces[seam.faceB].cell.position)
                          == 1;
        });

    EXPECT_EQ(
        sameLevelSeams(faces, seams, 1).size()
            + crossLevelSeams(faces, seams, 1).size(),
        static_cast<std::size_t>(touching));
}

TEST(VoxelWeaveTest, IsCornerSeam_TellsACornerFromAPlane)
{
    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    auto turning = 0;
    auto square = 0;

    for (const auto &seam : faceAdjacency(faces))
    {
        if (isCornerSeam(faces, seam))
        {
            EXPECT_NE(
                faceNormal(faces[seam.faceA].side),
                faceNormal(faces[seam.faceB].side));
            ++turning;
        }
        else
        {
            EXPECT_EQ(
                faceNormal(faces[seam.faceA].side),
                faceNormal(faces[seam.faceB].side));
            ++square;
        }
    }

    EXPECT_GT(turning, 0);
    EXPECT_GT(square, 0);
}

TEST(VoxelWeaveTest, IsCornerSeam_CallsEverySeamOfALoneVoxelACorner)
{
    const auto faces = visibleFacesOf(voxelsOf({VoxelCell{}}));

    for (const auto &seam : faceAdjacency(faces))
    {
        EXPECT_TRUE(isCornerSeam(faces, seam));
    }
}

TEST(VoxelWeaveTest, SolveTiles_KeepsATileFromARimItMayNotLieAt)
{
    constexpr Tile middleTile{.atlas = Atlas::Floor, .index = 5};
    constexpr Tile edgingTile{.atlas = Atlas::Floor, .index = 6};
    constexpr Tile sideTile{.atlas = Atlas::Wall, .index = 7};
    TileRules rules;

    for (const auto edge : kEveryTileEdge)
    {
        rules.allow(middleTile, edge, middleTile);
        rules.allow(middleTile, edge, edgingTile);
        rules.allow(edgingTile, edge, middleTile);
        rules.allow(edgingTile, edge, edgingTile);
        rules.setAllowsBoundary(edgingTile, edge, true);
        rules.setAllowsBoundary(sideTile, edge, true);
        rules.allow(sideTile, edge, sideTile);
    }

    const auto faces = visibleFacesOf(expandCubesToVoxels(demoCells()));
    const auto solution = solveTiles(faces, rules);

    ASSERT_TRUE(solution.tiles.has_value());

    const auto standing = faceAdjacency(faces);

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faceNormal(faces[index].side).y == 0.0F)
        {
            continue;
        }

        auto rimmedCount = 0;

        for (const auto &seam : standing)
        {
            if (seam.faceA == index || seam.faceB == index)
            {
                ++rimmedCount;
            }
        }

        if (rimmedCount < 4)
        {
            EXPECT_EQ((*solution.tiles)[index], edgingTile);
        }
    }
}

TEST(VoxelWeaveTest, SolveTiles_SaysSoWhereARimLeavesAFaceNoTile)
{
    constexpr Tile innerTile{.atlas = Atlas::Floor, .index = 5};
    constexpr Tile sideTile{.atlas = Atlas::Wall, .index = 7};
    TileRules rules;

    for (const auto edge : kEveryTileEdge)
    {
        rules.allow(innerTile, edge, innerTile);
        rules.allow(sideTile, edge, sideTile);
    }

    const auto faces = visibleFacesOf(voxelsOf({VoxelCell{}}));
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    EXPECT_FALSE(solution.tiles.has_value());
    EXPECT_EQ(solution.troubleFailure, SolveFailure::EmptyDomain);
}

TEST(VoxelWeaveTest, SolveTiles_SaysSoForAVoxelStandingOnItsOwn)
{
    const auto faces = visibleFacesOf(voxelsOf({VoxelCell{}}));
    const auto solution = solveTiles(faces, allowingAFew());

    ASSERT_TRUE(solution.tiles.has_value());
    EXPECT_EQ(solution.tiles->size(), faces.size());
}

TEST(VoxelWeaveTest, SolveTiles_TakesATileByWhatStandsBeyondItsCorner)
{
    Voxels ringVoxels;

    for (std::int32_t x = 0; x < 3; ++x)
    {
        for (std::int32_t z = 0; z < 3; ++z)
        {
            if (x == 1 && z == 1)
            {
                continue;
            }

            for (const auto &[position, material] :
                 expandCubesToVoxels(voxelsOf({VoxelCell{.position = {.x = x,
                     .z = z}}})))
            {
                ringVoxels[position] = material;
            }
        }
    }

    constexpr Tile filledTile{.atlas = Atlas::Floor, .index = 5};
    constexpr Tile hollowTile{.atlas = Atlas::Floor, .index = 6};
    constexpr Tile sideTile{.atlas = Atlas::Wall, .index = 7};
    TileRules rules;

    for (const auto edge : kEveryTileEdge)
    {
        for (const auto one : {filledTile, hollowTile})
        {
            for (const auto other : {filledTile, hollowTile})
            {
                rules.allow(one, edge, other);
            }

            rules.setAllowsBoundary(one, edge, true);
        }

        rules.allow(sideTile, edge, sideTile);
        rules.setAllowsBoundary(sideTile, edge, true);
    }

    rules.setCorner(filledTile, antwika::voxel::Corner::TopLeft, true);
    rules.setCorner(hollowTile, antwika::voxel::Corner::TopLeft, false);

    const auto faces = visibleFacesOf(ringVoxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    const std::set<antwika::voxelmap::FaceRef> standingRefs(
        faces.begin(), faces.end());
    auto flatTile = 0;

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faceNormal(faces[index].side).y <= 0.0F)
        {
            continue;
        }

        ++flatTile;

        const auto cell = faces[index].cell.position;
        const auto standingBeyond = standingRefs.contains(
            antwika::voxelmap::FaceRef{
                .cell = VoxelCell{.position = {.x = cell.x - 1, .y = cell.y,
                    .z = cell.z - 1}},
                .side = faces[index].side});

        EXPECT_EQ(
            (*solution.tiles)[index], standingBeyond ? filledTile : hollowTile);
    }

    EXPECT_GT(flatTile, 0);
}

TEST(VoxelWeaveTest, SolveTiles_DrawsAFaceFromTheTilesGivenToItsKind)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 4, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}}});
    const Tile stoneTile{.atlas = Atlas::Floor, .index = 0};
    const Tile poolTile{.atlas = Atlas::Floor, .index = 1};
    const Tile wallTile{.atlas = Atlas::Wall, .index = 0};
    const Tile sideTile{.atlas = Atlas::Wall, .index = 1};

    auto rules = allowingAmong({stoneTile, poolTile, wallTile, sideTile});

    rules.setKind(poolTile, Kind::Water);
    rules.setKind(sideTile, Kind::Water);

    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        const auto tileKind = rules.kindOf((*solution.tiles)[index]);

        EXPECT_EQ(tileKind, faces[index].cell.material.kind);
    }
}

TEST(VoxelWeaveTest, SolveTiles_LeavesAFaceAloneWithNoTileGivenToItsKind)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 4, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, allowingAFew(), CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());
    EXPECT_EQ(solution.skippedFaceCount, 6U);
}

TEST(VoxelWeaveTest, FaceAdjacency_BreaksWhereOneKindMeetsAnother)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}}});
    const auto faces = visibleFacesOf(voxels);

    for (const auto &seam : faceAdjacency(faces, CornerSeams::Ignored))
    {
        EXPECT_EQ(
            faces[seam.faceA].cell.material.kind,
            faces[seam.faceB].cell.material.kind);
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_HoldsASurfaceOfOneKindTogether)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}}});
    const auto faces = visibleFacesOf(voxels);
    const auto seams = faceAdjacency(faces, CornerSeams::Ignored);

    EXPECT_FALSE(seams.empty());
}

TEST(VoxelWeaveTest, SolveTiles_EndsASurfaceWhereAnotherKindBegins)
{
    using antwika::voxel::Kind;

    const Tile stoneTile{.atlas = Atlas::Floor, .index = 0};
    const Tile poolTile{.atlas = Atlas::Floor, .index = 1};
    const Tile wallTile{.atlas = Atlas::Wall, .index = 0};
    const Tile brinkTile{.atlas = Atlas::Wall, .index = 1};

    TileRules rules;

    for (const auto edge : kEveryTileEdge)
    {
        for (const auto tile : {stoneTile, wallTile})
        {
            rules.setAllowsBoundary(tile, edge, true);
            rules.allow(tile, edge, tile);
        }

        for (const auto tile : {poolTile, brinkTile})
        {
            rules.setAllowsBoundary(tile, edge, true);
            rules.allow(tile, edge, tile);
        }
    }

    rules.setKind(poolTile, Kind::Water);
    rules.setKind(brinkTile, Kind::Water);

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Water}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());
    EXPECT_EQ(solution.skippedFaceCount, 0U);

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        EXPECT_EQ(
            rules.kindOf((*solution.tiles)[index]),
            faces[index].cell.material.kind);
    }
}

TEST(VoxelWeaveTest, SolveTiles_DrawsEachLevelOfAFlightFromItsOwnTile)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::StairHalf;

    const Tile footTile{.atlas = Atlas::Wall, .index = 0};
    const Tile headTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({footTile, headTile, treadTile});

    for (const auto tile : {footTile, headTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setLevel(footTile, StairHalf::Lower);
    rules.setLevel(headTile, StairHalf::Upper);

    Voxels voxels;

    for (const auto &[position, material] :
         cubeVoxels(VoxelPosition{}, Kind::Ramp, VoxelPosition{.x = 1}))
    {
        voxels[position] = material;
    }

    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    std::size_t belowCount = 0;
    std::size_t aboveCount = 0;

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if ((*solution.tiles)[index].atlas != Atlas::Wall)
        {
            continue;
        }

        if (faces[index].levelHalf == StairHalf::Lower)
        {
            ++belowCount;

            EXPECT_EQ((*solution.tiles)[index], footTile);
        }

        if (faces[index].levelHalf == StairHalf::Upper)
        {
            ++aboveCount;

            EXPECT_EQ((*solution.tiles)[index], headTile);
        }
    }

    EXPECT_GT(belowCount, 0U);
    EXPECT_GT(aboveCount, 0U);
}

TEST(VoxelWeaveTest, SolveTiles_LaysASingledOutTileWithoutAskingTheRims)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxel::StairHalf;

    const Tile footTile{.atlas = Atlas::Wall, .index = 0};
    const Tile headTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({footTile, headTile, treadTile});

    for (const auto tile : {footTile, headTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setLevel(footTile, StairHalf::Lower);
    rules.setLevel(headTile, StairHalf::Upper);

    for (const auto edge : kEveryTileEdge)
    {
        rules.forbidAll(footTile, edge);
        rules.forbidAll(headTile, edge);
    }

    Voxels voxels;

    for (const auto &[position, material] :
         cubeVoxels(VoxelPosition{}, Kind::Ramp, VoxelPosition{.x = 1}))
    {
        voxels[position] = material;
    }

    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if ((*solution.tiles)[index].atlas != Atlas::Wall)
        {
            continue;
        }

        EXPECT_EQ(
            (*solution.tiles)[index],
            faces[index].levelHalf == StairHalf::Lower ? footTile : headTile);
    }
}

TEST(VoxelWeaveTest, SolveTiles_TakesNoTileDrawnForAnotherFlight)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::StairHalf;

    const Tile eastlyTile{.atlas = Atlas::Wall, .index = 0};
    const Tile westlyFootTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({eastlyTile, westlyFootTile, treadTile});

    for (const auto tile : {eastlyTile, westlyFootTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setFacing(eastlyTile, Facing::East);
    rules.setFacing(westlyFootTile, Facing::West);
    rules.setLevel(westlyFootTile, StairHalf::Lower);

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faces[index].cell.material.kind != Kind::Ramp
            || (*solution.tiles)[index].atlas != Atlas::Wall)
        {
            continue;
        }

        EXPECT_EQ((*solution.tiles)[index], eastlyTile);
    }
}

TEST(VoxelWeaveTest, SolveTiles_DrawsAFlightFromTheTilesDrawnForItsWay)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;

    const Tile eastlyTile{.atlas = Atlas::Wall, .index = 0};
    const Tile westlyTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({eastlyTile, westlyTile, treadTile});

    for (const auto tile : {eastlyTile, westlyTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setFacing(eastlyTile, Facing::East);
    rules.setFacing(westlyTile, Facing::West);

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faces[index].cell.material.kind != Kind::Ramp
            || (*solution.tiles)[index].atlas != Atlas::Wall)
        {
            continue;
        }

        EXPECT_EQ((*solution.tiles)[index], eastlyTile);
    }
}

TEST(VoxelWeaveTest, SolveTiles_LeavesAFlightAloneWithNoTileDrawnForItsWay)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;

    const Tile westlyTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({westlyTile, treadTile});

    for (const auto tile : {westlyTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setFacing(westlyTile, Facing::West);

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}}});
    const auto solution =
        solveTiles(visibleFacesOf(voxels), rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());
    EXPECT_GT(solution.skippedFaceCount, 0U);
}

TEST(VoxelWeaveTest, SolveTiles_KeepsAFlightOffTheTilesOfAnotherWayAbout)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;

    const Tile eastlyTile{.atlas = Atlas::Wall, .index = 0};
    const Tile plainTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({eastlyTile, plainTile, treadTile});

    for (const auto tile : {eastlyTile, plainTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setFacing(eastlyTile, Facing::East);

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = -1},
            .material = {.kind = Kind::Normal}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faces[index].cell.material.kind != Kind::Ramp
            || (*solution.tiles)[index].atlas != Atlas::Wall)
        {
            continue;
        }

        EXPECT_EQ((*solution.tiles)[index], plainTile);
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_BreaksWhereOneFlightMeetsAnother)
{
    using antwika::voxel::Facing;
    using antwika::solver::faceAdjacency;
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp, .facing = Facing::East}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp, .facing = Facing::North}}});
    const auto faces = visibleFacesOf(voxels);

    for (const auto &seam : faceAdjacency(faces, CornerSeams::Ignored))
    {
        EXPECT_EQ(
            faces[seam.faceA].climbPosition, faces[seam.faceB].climbPosition);
    }
}

TEST(VoxelWeaveTest, FaceAdjacency_HoldsOneFlightTogether)
{
    using antwika::voxel::Facing;
    using antwika::solver::faceAdjacency;
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp, .facing = Facing::East}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1},
            .material = {.kind = Kind::Ramp, .facing = Facing::East}}});

    EXPECT_GT(
        faceAdjacency(visibleFacesOf(voxels), CornerSeams::Ignored)
            .size(),
        0U);
}

TEST(VoxelWeaveTest, SolveTiles_LaysTwoFlightsThatMeetWithNoRuleBetween)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;

    const Tile eastlyTile{.atlas = Atlas::Wall, .index = 0};
    const Tile northlyTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({eastlyTile, northlyTile, treadTile});

    for (const auto tile : {eastlyTile, northlyTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setFacing(eastlyTile, Facing::East);
    rules.setFacing(northlyTile, Facing::North);

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp, .facing = Facing::East}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp, .facing = Facing::North}}});

    EXPECT_TRUE(
        solveTiles(visibleFacesOf(voxels), rules, CornerSeams::Ignored)
            .tiles.has_value());
}

TEST(VoxelWeaveTest, SolveTiles_DressesTheSideOfAFlightApartFromItsFronts)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::StairPart;
    using antwika::voxelmap::stairPartOf;

    const Tile frontTile{.atlas = Atlas::Wall, .index = 0};
    const Tile flankTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({frontTile, flankTile, treadTile});

    for (const auto tile : {frontTile, flankTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setPart(flankTile, StairPart::Side);

    const auto voxels = voxelsOf({
        VoxelCell{.material = {.kind = Kind::Ramp, .facing = Facing::East}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solvedTiles =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solvedTiles.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        const auto part =
            stairPartOf(faces[index].climbPosition, faces[index].side);

        if (part == StairPart::Side)
        {
            EXPECT_EQ((*solvedTiles.tiles)[index], flankTile);
        }

        if (part == StairPart::Front)
        {
            EXPECT_EQ((*solvedTiles.tiles)[index], frontTile);
        }
    }
}

TEST(VoxelWeaveTest, SolveTiles_OffersASideTileWithNoRuleAgainstItsEdges)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::StairPart;
    using antwika::voxelmap::stairPartOf;

    const Tile frontTile{.atlas = Atlas::Wall, .index = 0};
    const Tile flankTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({frontTile, treadTile});

    for (const auto tile : {frontTile, flankTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    rules.setPart(flankTile, StairPart::Side);

    const auto voxels = voxelsOf({
        VoxelCell{.material = {.kind = Kind::Ramp, .facing = Facing::East}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solvedTiles =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solvedTiles.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        const auto part =
            stairPartOf(faces[index].climbPosition, faces[index].side);

        if (part == StairPart::Side)
        {
            EXPECT_EQ((*solvedTiles.tiles)[index], flankTile);
        }

        if (part == StairPart::Front)
        {
            EXPECT_EQ((*solvedTiles.tiles)[index], frontTile);
        }
    }
}

TEST(VoxelWeaveTest, SolveTiles_LetsOneTileServeAWholeFlightUnasked)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::StairPart;
    using antwika::voxelmap::stairPartOf;

    const Tile uprightTile{.atlas = Atlas::Wall, .index = 0};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({uprightTile, treadTile});

    for (const auto tile : {uprightTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    const auto voxels = voxelsOf({
        VoxelCell{.material = {.kind = Kind::Ramp, .facing = Facing::East}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solvedTiles =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solvedTiles.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (stairPartOf(faces[index].climbPosition, faces[index].side)
            != StairPart::Any)
        {
            EXPECT_EQ((*solvedTiles.tiles)[index], uprightTile);
        }
    }
}

TEST(VoxelWeaveTest, SolveTiles_SaysWhereARimLeavesAFaceNoTile)
{
    constexpr Tile innerTile{.atlas = Atlas::Floor, .index = 5};
    constexpr Tile sideTile{.atlas = Atlas::Wall, .index = 7};
    TileRules rules;

    for (const auto edge : kEveryTileEdge)
    {
        rules.allow(innerTile, edge, innerTile);
        rules.allow(sideTile, edge, sideTile);
    }

    const auto solution =
        solveTiles(visibleFacesOf(voxelsOf({VoxelCell{}})), rules,
            CornerSeams::Ignored);

    ASSERT_FALSE(solution.tiles.has_value());
    ASSERT_FALSE(solution.conflictFaces.empty());
    EXPECT_EQ(solution.conflictFaces.front().cell, VoxelCell{});
}

TEST(VoxelWeaveTest, SolveTiles_SaysWhereOnlyWhereItWillNotLay)
{
    const auto solution =
        solveTiles(visibleFacesOf(voxelsOf({VoxelCell{}})), allowingAFew());

    ASSERT_TRUE(solution.tiles.has_value());
    EXPECT_TRUE(solution.conflictFaces.empty());
}

TEST(VoxelWeaveTest, WeaveErrorMessage_NamesWhereTheTroubleGathers)
{
    constexpr Tile innerTile{.atlas = Atlas::Floor, .index = 5};
    constexpr Tile sideTile{.atlas = Atlas::Wall, .index = 7};
    TileRules rules;

    for (const auto edge : kEveryTileEdge)
    {
        rules.allow(innerTile, edge, innerTile);
        rules.allow(sideTile, edge, sideTile);
    }

    const auto faces = visibleFacesOf(voxelsOf({VoxelCell{}}));
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);
    const auto message =
        weaveErrorMessage(
            faces, rules, solution, CornerSeams::Ignored);

    EXPECT_NE(message.find("look at the"), std::string::npos);
    EXPECT_NE(message.find("(0,0,0)"), std::string::npos);
}

TEST(VoxelWeaveTest, SolveTiles_LaysABorderedTileWhereASurfaceReallyEnds)
{
    const Tile borderedTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({treadTile});

    for (const auto edge : kEveryTileEdge)
    {
        rules.setAllowsBoundary(borderedTile, edge, true);
    }

    const auto faces = visibleFacesOf(voxelsOf({VoxelCell{}}));
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if ((*solution.tiles)[index].atlas == Atlas::Wall)
        {
            EXPECT_EQ((*solution.tiles)[index], borderedTile);
        }
    }
}

TEST(VoxelWeaveTest, SolveTiles_StopsAWallWhereAFlightStepsAcrossIt)
{
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;

    const Tile borderedTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({treadTile});

    for (const auto edge : kEveryTileEdge)
    {
        rules.setAllowsBoundary(borderedTile, edge, true);
    }

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp, .facing = Facing::East}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    auto stoodCount = 0;

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faces[index].cell.material.kind != Kind::Normal
            || (*solution.tiles)[index].atlas != Atlas::Wall)
        {
            continue;
        }

        EXPECT_EQ((*solution.tiles)[index], borderedTile);
        ++stoodCount;
    }

    EXPECT_GT(stoodCount, 0);
}

TEST(VoxelWeaveTest, SolveTiles_LeavesAFlightsOwnShapeToTheSetItWasDrawnFor)
{
    using antwika::voxel::Kind;

    const Tile borderedTile{.atlas = Atlas::Wall, .index = 1};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({treadTile});

    for (const auto edge : kEveryTileEdge)
    {
        rules.setAllowsBoundary(borderedTile, edge, true);
    }

    for (const auto tile : {borderedTile, treadTile})
    {
        rules.setKind(tile, Kind::Ramp);
    }

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    auto stoodCount = 0;

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faces[index].cell.material.kind != Kind::Ramp
            || (*solution.tiles)[index].atlas != Atlas::Wall)
        {
            continue;
        }

        EXPECT_EQ((*solution.tiles)[index], borderedTile);
        ++stoodCount;
    }

    EXPECT_GT(stoodCount, 0);
}

TEST(VoxelWeaveTest, SolveTiles_SaysWhichSeamTheTilesLeftCannotMeetAlong)
{
    using antwika::solver::weaveErrorMessage;

    const Tile leftlyTile{.atlas = Atlas::Wall, .index = 1};
    const Tile rightlyTile{.atlas = Atlas::Wall, .index = 2};
    const Tile middleTile{.atlas = Atlas::Wall, .index = 3};
    const Tile loneTile{.atlas = Atlas::Wall, .index = 4};
    const Tile treadTile{.atlas = Atlas::Floor, .index = 0};

    auto rules = allowingAmong({treadTile});

    for (const auto edge : kEveryTileEdge)
    {
        rules.setAllowsBoundary(loneTile, edge, true);
        rules.setAllowsBoundary(middleTile, edge, false);
        rules.allow(middleTile, edge, leftlyTile);
        rules.allow(middleTile, edge, rightlyTile);
        rules.allow(middleTile, edge, middleTile);
    }

    for (const auto sideTile : {Side::Top, Side::Bottom})
    {
        for (const auto edge : {EdgeKind::Interior, EdgeKind::Boundary})
        {
            rules.setAllowsBoundary(leftlyTile, TileEdge{sideTile, edge}, true);
            rules.setAllowsBoundary(
                rightlyTile,
                TileEdge{sideTile, edge},
                true);
        }
    }

    for (const auto edge : {EdgeKind::Interior, EdgeKind::Boundary})
    {
        rules.setAllowsBoundary(leftlyTile, TileEdge{Side::Left, edge}, true);
        rules.allow(leftlyTile, TileEdge{Side::Right, edge}, middleTile);
        rules.setAllowsBoundary(rightlyTile, TileEdge{Side::Right, edge}, true);
        rules.allow(rightlyTile, TileEdge{Side::Left, edge}, middleTile);
    }

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_FALSE(solution.tiles.has_value());
    EXPECT_EQ(solution.troubleFailure, SolveFailure::IncompatibleEdge);
    EXPECT_FALSE(solution.conflictFaces.empty());

    const auto message =
        weaveErrorMessage(
            faces, rules, solution, CornerSeams::Ignored);

    EXPECT_NE(message.find("either face is left with"), std::string::npos);
    EXPECT_NE(message.find("look at the"), std::string::npos);
}

TEST(VoxelWeaveTest, SolveTiles_TurnsACornerWhereTheOnlyWayRoundIsPastRamps)
{
    using antwika::voxel::Corner;
    using antwika::voxel::kEveryCorner;
    using antwika::voxel::Kind;

    const Tile groundTile{.atlas = Atlas::Floor, .index = 0};
    const Tile sideTile{.atlas = Atlas::Wall, .index = 0};

    auto rules = allowingAmong({groundTile, sideTile});

    for (const auto corner : kEveryCorner)
    {
        rules.setCorner(groundTile, corner, false);
    }

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1},
            .material = {.kind = Kind::Ramp}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 1},
            .material = {.kind = Kind::Normal}}});
    const auto solution =
        solveTiles(visibleFacesOf(voxels), rules, CornerSeams::Ignored);

    EXPECT_TRUE(solution.tiles.has_value());
}

TEST(VoxelWeaveTest, SolveTiles_RunsOnRoundACornerTheGroundItselfReaches)
{
    using antwika::voxel::kEveryCorner;
    using antwika::voxel::Kind;

    const Tile groundTile{.atlas = Atlas::Floor, .index = 0};
    const Tile sideTile{.atlas = Atlas::Wall, .index = 0};

    auto rules = allowingAmong({groundTile, sideTile});

    for (const auto corner : kEveryCorner)
    {
        rules.setCorner(groundTile, corner, false);
    }

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 0, .y = 0, .z = 1},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 1},
            .material = {.kind = Kind::Normal}}});
    const auto solution =
        solveTiles(visibleFacesOf(voxels), rules, CornerSeams::Ignored);

    EXPECT_FALSE(solution.tiles.has_value());
    EXPECT_EQ(solution.troubleFailure, SolveFailure::EmptyDomain);
}

TEST(VoxelWeaveTest, SolveTiles_StopsAFloorAtTheHeadOfAStair)
{
    using antwika::voxel::Kind;

    const Tile borderedTile{.atlas = Atlas::Floor, .index = 1};
    const Tile sideTile{.atlas = Atlas::Wall, .index = 0};

    auto rules = allowingAmong({sideTile});

    for (const auto edge : kEveryTileEdge)
    {
        rules.setAllowsBoundary(borderedTile, edge, true);
    }

    const auto voxels = voxelsOf({
        VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = Kind::Normal}},
        VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = Kind::Ramp}}});
    const auto faces = visibleFacesOf(voxels);
    const auto solution =
        solveTiles(faces, rules, CornerSeams::Ignored);

    ASSERT_TRUE(solution.tiles.has_value());

    for (std::size_t index = 0; index < faces.size(); ++index)
    {
        if (faces[index].cell.material.kind == Kind::Normal
            && (*solution.tiles)[index].atlas == Atlas::Floor)
        {
            EXPECT_EQ((*solution.tiles)[index], borderedTile);
        }
    }
}

