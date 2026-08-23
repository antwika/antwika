#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <vector>

#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tile/TileRules.hpp>

#include <antwika/decor/Decor.hpp>

namespace
{

    using antwika::tilemap::Atlas;
    using antwika::decor::decorFrameAt;
    using antwika::decor::decorOf;
    using antwika::decor::DecorTile;
    using antwika::decor::getSolveDecor;
    using antwika::voxelmap::visibleFacesOf;
    using antwika::voxelmap::getDefaultTiles;
    using antwika::decor::kDecorPaceTick;
    using antwika::tilemap::Tile;
    using antwika::tile::TileRules;
    using antwika::decor::getPreviewNeighbourhood;
    using antwika::decor::getWithBaseToggled;
    using antwika::decor::getWithDecorToggled;
    using antwika::decor::getWithFrameAdded;
    using antwika::decor::getWithFrameSet;
    using antwika::decor::getFrameWidget;
    using antwika::decor::hasAnimatedDecor;
    using antwika::decor::getMemberWidget;

    constexpr Tile kOneTile{.atlas = Atlas::Floor, .index = 1};
    constexpr Tile kOtherTile{.atlas = Atlas::Floor, .index = 2};

    TEST(DecorTest, WithDecorToggled_MarksATileAndUnmarksIt)
    {
        const auto toggledDecor =
            getWithDecorToggled(std::vector<DecorTile>{}, kOneTile);

        ASSERT_NE(decorOf(toggledDecor, kOneTile), nullptr);
        EXPECT_EQ(decorOf(toggledDecor, kOneTile)->frameTiles.size(), 1U);

        const auto untoggledDecor = getWithDecorToggled(toggledDecor, kOneTile);

        EXPECT_EQ(decorOf(untoggledDecor, kOneTile), nullptr);
    }

    TEST(DecorTest, WithBaseToggled_AllowsABaseAndTakesItBack)
    {
        auto decor =
            getWithDecorToggled(std::vector<DecorTile>{}, kOneTile);

        decor = getWithBaseToggled(decor, kOneTile, kOtherTile);

        ASSERT_EQ(decorOf(decor, kOneTile)->allowedBaseTiles.size(), 1U);
        EXPECT_EQ(
            decorOf(decor, kOneTile)->allowedBaseTiles.front(), kOtherTile);

        decor = getWithBaseToggled(decor, kOneTile, kOtherTile);

        EXPECT_TRUE(decorOf(decor, kOneTile)->allowedBaseTiles.empty());
    }

    TEST(DecorTest, WithFrameSet_LeavesTheFirstFrameTheTileItself)
    {
        auto decor =
            getWithDecorToggled(std::vector<DecorTile>{}, kOneTile);

        decor = getWithFrameAdded(decor, kOneTile);
        decor = getWithFrameSet(decor, kOneTile, 1, kOtherTile);
        decor = getWithFrameSet(decor, kOneTile, 0, kOtherTile);

        ASSERT_EQ(decorOf(decor, kOneTile)->frameTiles.size(), 2U);
        EXPECT_EQ(decorOf(decor, kOneTile)->frameTiles[0], kOneTile);
        EXPECT_EQ(decorOf(decor, kOneTile)->frameTiles[1], kOtherTile);
    }

    TEST(DecorTest, WithFrequencySet_SaysHowOftenAndHoldsTheCeiling)
    {
        auto decor =
            getWithDecorToggled(std::vector<DecorTile>{}, kOneTile);

        EXPECT_EQ(
            decorOf(decor, kOneTile)->frequency,
            antwika::decor::kFullFrequency);

        decor = antwika::decor::getWithFrequencySet(decor, kOneTile, 25);

        EXPECT_EQ(decorOf(decor, kOneTile)->frequency, 25);

        decor = antwika::decor::getWithFrequencySet(decor, kOneTile, 250);

        EXPECT_EQ(
            decorOf(decor, kOneTile)->frequency,
            antwika::decor::kFullFrequency);
    }

    TEST(DecorTest, SolveDecor_LaysNothingAtAFrequencyOfNought)
    {
        const auto voxels = antwika::voxel::getExpandCubesToVoxels(
            antwika::voxel::voxelsOf({antwika::voxel::VoxelCell{}}));
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getDefaultTiles(faces);
        auto decor = getWithDecorToggled({}, kOneTile);

        for (const auto tile : drawnTiles)
        {
            if (decorOf(decor, kOneTile)->allowedBaseTiles.empty()
                || decorOf(decor, kOneTile)->allowedBaseTiles.back() != tile)
            {
                decor = getWithBaseToggled(decor, kOneTile, tile);
            }
        }

        const auto solvedDecor =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);

        EXPECT_FALSE(solvedDecor.empty());

        decor = antwika::decor::getWithFrequencySet(decor, kOneTile, 0);

        const auto bare =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);

        EXPECT_TRUE(bare.empty());
    }

    TEST(DecorTest, WithWeightSet_SaysHowStronglyAndHoldsTheCeiling)
    {
        auto decor =
            getWithDecorToggled(std::vector<DecorTile>{}, kOneTile);

        EXPECT_EQ(
            decorOf(decor, kOneTile)->weight,
            antwika::decor::kFullFrequency);

        decor = antwika::decor::getWithWeightSet(decor, kOneTile, 25);

        EXPECT_EQ(decorOf(decor, kOneTile)->weight, 25);

        decor = antwika::decor::getWithWeightSet(decor, kOneTile, 250);

        EXPECT_EQ(
            decorOf(decor, kOneTile)->weight,
            antwika::decor::kFullFrequency);
    }

    TEST(DecorTest, SolveDecor_PassesOverADecorOfNoWeight)
    {
        antwika::voxel::Voxels cubeVoxels;

        for (std::int32_t x = 0; x < 4; ++x)
        {
            for (std::int32_t z = 0; z < 4; ++z)
            {
                cubeVoxels[antwika::voxel::VoxelPosition{.x = x, .z = z}] =
                    antwika::voxel::VoxelMaterial{};
            }
        }

        const auto voxels =
            antwika::voxel::getExpandCubesToVoxels(cubeVoxels);
        const auto faces = visibleFacesOf(voxels);
        constexpr Tile kBaseTile{.atlas = Atlas::Floor, .index = 9};
        constexpr Tile kWantedTile{.atlas = Atlas::Floor, .index = 26};
        constexpr Tile kShunnedTile{.atlas = Atlas::Floor, .index = 27};

        std::vector<Tile> drawnTiles(faces.size(), kBaseTile);
        std::vector<DecorTile> decor;

        for (const auto offeredTile : {kWantedTile, kShunnedTile})
        {
            decor = getWithDecorToggled(decor, offeredTile);
            decor = getWithBaseToggled(decor, offeredTile, kBaseTile);
        }

        decor = antwika::decor::getWithWeightSet(decor, kShunnedTile, 0);

        const auto solvedDecor =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);

        EXPECT_FALSE(solvedDecor.empty());

        for (const auto &[face, tile] : solvedDecor)
        {
            EXPECT_EQ(tile, kWantedTile);
        }
    }

    TEST(DecorTest, SolveDecor_MixesTheDecorABaseOffersMoreThanOneOf)
    {
        antwika::voxel::Voxels cubeVoxels;

        for (std::int32_t x = 0; x < 4; ++x)
        {
            for (std::int32_t z = 0; z < 4; ++z)
            {
                cubeVoxels[antwika::voxel::VoxelPosition{.x = x, .z = z}] =
                    antwika::voxel::VoxelMaterial{};
            }
        }

        const auto voxels =
            antwika::voxel::getExpandCubesToVoxels(cubeVoxels);
        const auto faces = visibleFacesOf(voxels);
        constexpr Tile kBaseTile{.atlas = Atlas::Floor, .index = 9};

        std::vector<Tile> drawnTiles(faces.size(), kBaseTile);
        std::vector<DecorTile> decor;

        for (std::uint16_t offeredIndex : {26, 27, 28})
        {
            const Tile offeredTile{
                .atlas = Atlas::Floor, .index = offeredIndex};

            decor = getWithDecorToggled(decor, offeredTile);
            decor = getWithBaseToggled(decor, offeredTile, kBaseTile);
        }

        const auto solvedDecor =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);

        std::set<std::uint16_t> seenTiles;

        for (const auto &[face, tile] : solvedDecor)
        {
            seenTiles.insert(tile.index);
        }

        EXPECT_EQ(
            seenTiles,
            (std::set<std::uint16_t>{26, 27, 28}));
    }

    TEST(DecorTest, WithDecorLayerSet_MovesADecorToAnotherLayer)
    {
        auto decor =
            getWithDecorToggled(std::vector<DecorTile>{}, kOneTile);

        EXPECT_EQ(decorOf(decor, kOneTile)->layer, 1U);

        decor =
            antwika::decor::getWithDecorLayerSet(decor, kOneTile, 3);

        EXPECT_EQ(decorOf(decor, kOneTile)->layer, 3U);
        EXPECT_EQ(
            antwika::decor::getWithDecorLayerSet(decor, kOtherTile, 2),
            decor);
    }

    TEST(DecorTest, SolveDecorLayers_LaysTheLayersOverOneAnother)
    {
        const auto voxels = antwika::voxel::getExpandCubesToVoxels(
            antwika::voxel::voxelsOf({antwika::voxel::VoxelCell{}}));
        const auto faces = visibleFacesOf(voxels);
        constexpr Tile kBaseTile{.atlas = Atlas::Floor, .index = 9};
        constexpr Tile kUnderTile{.atlas = Atlas::Floor, .index = 26};
        constexpr Tile kOverTile{.atlas = Atlas::Floor, .index = 27};

        std::vector<Tile> drawnTiles(faces.size(), kBaseTile);
        std::vector<DecorTile> decor;

        decor = getWithDecorToggled(decor, kUnderTile, 1);
        decor = getWithBaseToggled(decor, kUnderTile, kBaseTile);
        decor = getWithDecorToggled(decor, kOverTile, 2);
        decor = getWithBaseToggled(decor, kOverTile, kBaseTile);

        const auto layers = antwika::decor::getSolveDecorLayers(
            faces, drawnTiles, decor, TileRules{}, 0);

        ASSERT_EQ(layers.size(), 2U);
        EXPECT_EQ(layers[0].first, 1U);
        EXPECT_EQ(layers[1].first, 2U);
        EXPECT_FALSE(layers[0].second.empty());
        EXPECT_FALSE(layers[1].second.empty());

        auto sharesTile = false;

        for (const auto &[face, tile] : layers[0].second)
        {
            sharesTile = sharesTile
                     || layers[1].second.contains(face);
        }

        EXPECT_TRUE(sharesTile);
    }

    TEST(DecorTest, DecorFrameAt_WalksTheFramesAsTheClockRuns)
    {
        auto decor =
            getWithDecorToggled(std::vector<DecorTile>{}, kOneTile);

        decor = getWithFrameAdded(decor, kOneTile);
        decor = getWithFrameSet(decor, kOneTile, 1, kOtherTile);

        const auto &decorTile = *decorOf(decor, kOneTile);

        EXPECT_EQ(decorFrameAt(decorTile, 0), kOneTile);
        EXPECT_EQ(decorFrameAt(decorTile, kDecorPaceTick), kOtherTile);
        EXPECT_EQ(
            decorFrameAt(decorTile, 2 * kDecorPaceTick), kOneTile);
    }

    TEST(DecorTest, PreviewNeighbourhood_PinsTheMiddleAndFillsTheSquare)
    {
        const TileRules rules;
        const auto preview =
            getPreviewNeighbourhood(rules, kOneTile, 3, 7);

        ASSERT_TRUE(preview.has_value());
        ASSERT_EQ(preview->size(), 9U);
        EXPECT_EQ(preview->at(4), kOneTile);
    }


    [[nodiscard]] std::vector<Tile> getFacedWith(
        const std::vector<antwika::voxelmap::FaceRef> &faces,
        const Tile wallTile,
        const Tile topTile)
    {
        std::vector<Tile> tiles;

        for (const auto &face : faces)
        {
            tiles.push_back(
                antwika::gfx::Vec3(
                    antwika::voxelmap::getFaceNormal(face.side))
                            .y
                        == 0.0F
                         ? wallTile
                         : topTile);
        }

        return tiles;
    }

    constexpr Tile kBrickTile{.atlas = Atlas::Wall, .index = 3};

    constexpr Tile kMossTile{.atlas = Atlas::Wall, .index = 7};

    TEST(DecorTest, SolveDecor_DressesAWallItsBasesAllow)
    {
        const auto voxels = antwika::voxel::voxelsOf({
            antwika::voxel::VoxelCell{}});
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kMossTile);

        decor = getWithBaseToggled(decor, kMossTile, kBrickTile);

        const auto placements =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);

        EXPECT_EQ(placements.size(), 4U);

        for (const auto &[faceIndex, tile] : placements)
        {
            EXPECT_EQ(
                antwika::gfx::Vec3(
                    antwika::voxelmap::getFaceNormal(
                        faces.at(faceIndex).side))
                    .y,
                0.0F);
            EXPECT_EQ(tile, kMossTile);
        }
    }

    TEST(DecorTest, SolveDecor_LeavesFlatDecorOffWalls)
    {
        const auto voxels = antwika::voxel::voxelsOf({
            antwika::voxel::VoxelCell{}});
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOtherTile);
        auto decor = getWithDecorToggled({}, kOneTile);

        decor = getWithBaseToggled(decor, kOneTile, kBrickTile);

        EXPECT_TRUE(
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0)
                .empty());
    }

    TEST(DecorTest, SolveDecor_HoldsWallSeamsToTheRules)
    {
        const auto voxels = antwika::voxel::voxelsOf({
            antwika::voxel::VoxelCell{},
            antwika::voxel::VoxelCell{.position = {.x = 1}}});
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kMossTile);

        decor = getWithBaseToggled(decor, kMossTile, kBrickTile);

        TileRules rules;

        rules.forbidAll(
            kMossTile,
            antwika::tilemap::TileEdge{
                .side = antwika::voxel::Side::Right,
                .edge = antwika::voxel::EdgeKind::Boundary});
        rules.forbidAll(
            kMossTile,
            antwika::tilemap::TileEdge{
                .side = antwika::voxel::Side::Right,
                .edge = antwika::voxel::EdgeKind::Interior});

        const auto placements =
            getSolveDecor(faces, drawnTiles, decor, rules, 0);
        std::size_t front = 0;

        for (const auto &[faceIndex, tile] : placements)
        {
            front += faces.at(faceIndex).side == 0 ? 1U : 0U;
        }

        EXPECT_LT(front, 2U);
    }

    TEST(DecorTest, SolveDecor_KeepsWallPlanesApart)
    {
        const auto voxels = antwika::voxel::voxelsOf({
            antwika::voxel::VoxelCell{}});
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kMossTile);

        decor = getWithBaseToggled(decor, kMossTile, kBrickTile);

        TileRules rules;

        rules.forbidAll(
            kMossTile,
            antwika::tilemap::TileEdge{
                .side = antwika::voxel::Side::Right,
                .edge = antwika::voxel::EdgeKind::Boundary});
        rules.forbidAll(
            kMossTile,
            antwika::tilemap::TileEdge{
                .side = antwika::voxel::Side::Right,
                .edge = antwika::voxel::EdgeKind::Interior});

        EXPECT_EQ(
            getSolveDecor(faces, drawnTiles, decor, rules, 0).size(),
            4U);
    }

    TEST(DecorTest, SolveDecor_ThinsWallsByFrequencyApart)
    {
        antwika::voxel::Voxels voxels;

        for (std::int32_t x = 0; x < 16; ++x)
        {
            voxels[antwika::voxel::VoxelPosition{.x = x, .z = x * 3}] =
                    antwika::voxel::VoxelMaterial{};
        }

        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kMossTile);

        decor = getWithBaseToggled(decor, kMossTile, kBrickTile);
        decor = antwika::decor::getWithFrequencySet(decor, kMossTile, 50);

        const auto placements =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);

        EXPECT_GT(placements.size(), 0U);
        EXPECT_LT(placements.size(), 64U);
        EXPECT_EQ(
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0),
            placements);
    }


    TEST(DecorTest, WithSpanSet_GrowsRowByRowAndKeepsTheOverlap)
    {
        auto decor = getWithDecorToggled({}, kOneTile);

        decor = antwika::decor::getWithSpanSet(decor, kOneTile, 2, 1);
        decor = antwika::decor::getWithMemberSet(
            decor, kOneTile, 1, kOtherTile);
        decor = antwika::decor::getWithSpanSet(decor, kOneTile, 2, 2);

        const auto &decorTile = *decorOf(decor, kOneTile);

        ASSERT_EQ(decorTile.spanTiles.size(), 4U);
        EXPECT_EQ(decorTile.spanTiles.at(0), kOneTile);
        EXPECT_EQ(decorTile.spanTiles.at(1), kOtherTile);
        EXPECT_EQ(decorTile.spanTiles.at(2), kOneTile);
        EXPECT_EQ(decorTile.spanTiles.at(3), kOneTile);
    }

    TEST(DecorTest, WithSpanSet_HoldsTheSpanToItsWidest)
    {
        auto decor = getWithDecorToggled({}, kOneTile);

        decor = antwika::decor::getWithSpanSet(decor, kOneTile, 9, 0);

        const auto &decorTile = *decorOf(decor, kOneTile);

        EXPECT_EQ(
            decorTile.width, antwika::decor::kMaxDecorSpan);
        EXPECT_EQ(decorTile.height, 1);
    }

    TEST(DecorTest, WithSpanSet_TrimsTheFramesOfASpannedDecor)
    {
        auto decor = getWithDecorToggled({}, kOneTile);

        decor = getWithFrameAdded(decor, kOneTile);
        decor = antwika::decor::getWithSpanSet(decor, kOneTile, 2, 1);

        EXPECT_EQ(decorOf(decor, kOneTile)->frameTiles.size(), 1U);
    }

    TEST(DecorTest, WithMemberSet_KeepsTheAnchorAndTheAtlas)
    {
        auto decor = getWithDecorToggled({}, kOneTile);

        decor = antwika::decor::getWithSpanSet(decor, kOneTile, 2, 1);
        decor = antwika::decor::getWithMemberSet(
            decor, kOneTile, 0, kOtherTile);
        decor = antwika::decor::getWithMemberSet(
            decor, kOneTile, 1, kBrickTile);

        const auto &decorTile = *decorOf(decor, kOneTile);

        EXPECT_EQ(decorTile.spanTiles.at(0), kOneTile);
        EXPECT_EQ(decorTile.spanTiles.at(1), kOneTile);
    }

    TEST(DecorTest, SolveDecor_StampsAWholeFootprintOrNothing)
    {
        const auto voxels = antwika::voxel::voxelsOf({
            antwika::voxel::VoxelCell{},
            antwika::voxel::VoxelCell{.position = {.x = 1}}});
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kMossTile);

        decor = getWithBaseToggled(decor, kMossTile, kBrickTile);
        decor = antwika::decor::getWithSpanSet(decor, kMossTile, 2, 1);
        decor = antwika::decor::getWithMemberSet(
            decor,
            kMossTile,
            1,
            Tile{.atlas = Atlas::Wall, .index = 8});

        const auto placements =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);
        std::size_t front = 0;
        std::size_t second = 0;

        for (const auto &[faceIndex, tile] : placements)
        {
            if (faces.at(faceIndex).side != 0)
            {
                continue;
            }

            front += tile == kMossTile ? 1U : 0U;
            second +=
                tile
                        == Tile{
                            .atlas = Atlas::Wall,
                            .index = 8}
                                   ? 1U
                                   : 0U;
        }

        EXPECT_EQ(front, 1U);
        EXPECT_EQ(second, 1U);
    }

    TEST(DecorTest, SolveDecor_SkipsAStampItsGroundCannotHold)
    {
        const auto voxels = antwika::voxel::voxelsOf({
            antwika::voxel::VoxelCell{}});
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kMossTile);

        decor = getWithBaseToggled(decor, kMossTile, kBrickTile);
        decor = antwika::decor::getWithSpanSet(decor, kMossTile, 3, 1);

        EXPECT_TRUE(
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0)
                .empty());
    }

    TEST(DecorTest, SolveDecor_LaysTheSameStampsUnderOneSeed)
    {
        antwika::voxel::Voxels voxels;

        for (std::int32_t x = 0; x < 8; ++x)
        {
            for (std::int32_t z = 0; z < 8; ++z)
            {
                voxels[antwika::voxel::VoxelPosition{.x = x, .z = z}] =
                    antwika::voxel::VoxelMaterial{};
            }
        }

        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kOneTile);

        decor = getWithBaseToggled(decor, kOneTile, kOneTile);
        decor = antwika::decor::getWithSpanSet(decor, kOneTile, 2, 2);
        decor =
            antwika::decor::getWithFrequencySet(decor, kOneTile, 30);

        const auto placements =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 7);

        EXPECT_FALSE(placements.empty());
        EXPECT_EQ(placements.size() % 4, 0U);
        EXPECT_EQ(
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 7),
            placements);
    }


    TEST(DecorTest, SolveDecor_DressesAWaterTopItsBasesAllow)
    {
        const auto voxels = antwika::voxel::voxelsOf({
            antwika::voxel::VoxelCell{
                .material = {.kind = antwika::voxel::Kind::Water}}});
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kOtherTile);

        decor = getWithBaseToggled(decor, kOtherTile, kOneTile);

        const auto placements =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);

        EXPECT_FALSE(placements.empty());

        for (const auto &[faceIndex, tile] : placements)
        {
            EXPECT_GT(
                antwika::gfx::Vec3(
                    antwika::voxelmap::getFaceNormal(
                        faces.at(faceIndex).side))
                    .y,
                0.0F);
        }
    }

    TEST(DecorTest, SolveDecor_DressesALadderWallItsBasesAllow)
    {
        const auto voxels = antwika::voxel::voxelsOf({
            antwika::voxel::VoxelCell{
                .material = {.kind = antwika::voxel::Kind::Ladder}}});
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getFacedWith(faces, kBrickTile, kOneTile);
        auto decor = getWithDecorToggled({}, kMossTile);

        decor = getWithBaseToggled(decor, kMossTile, kBrickTile);

        EXPECT_FALSE(
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0)
                .empty());
    }

    TEST(DecorTest, DecorMesh_LaysAFlightStepByStep)
    {
        const std::vector<antwika::voxelmap::FaceRef> faces{
            antwika::voxelmap::FaceRef{
                .cell =
                    antwika::voxel::VoxelCell{
                        .material = {.kind = antwika::voxel::Kind::Ramp}},
                .side = 4,
                .climbPosition = antwika::voxel::VoxelPosition{.x = 1}}};
        const std::map<std::size_t, Tile> placedTiles{{0, kOneTile}};
        const auto mesh = antwika::decor::getDecorMesh(
            faces, placedTiles, getWithDecorToggled({}, kOneTile), 0);
        std::size_t treads = 0;

        for (const auto &quad :
             antwika::voxel::getStairQuads(
                 antwika::voxel::VoxelPosition{.x = 1}))
        {
            treads += quad.side == 4 ? 1U : 0U;
        }

        EXPECT_GT(treads, 1U);
        EXPECT_EQ(mesh.vertices.size(), treads * 4);
        EXPECT_EQ(mesh.indices.size(), treads * 6);
    }


    TEST(DecorTest, CompactedDecor_LetsUntouchedRecordsGo)
    {
        auto decor = getWithDecorToggled({}, kOneTile);

        decor = getWithDecorToggled(decor, kOtherTile);
        decor = getWithBaseToggled(decor, kOtherTile, kBrickTile);
        decor = getWithDecorToggled(decor, kMossTile);
        decor = antwika::decor::getWithFrequencySet(decor, kMossTile, 40);

        const auto compactedList = antwika::decor::getCompactedDecor(decor);

        ASSERT_EQ(compactedList.size(), 2U);
        EXPECT_EQ(decorOf(compactedList, kOneTile), nullptr);
        EXPECT_NE(decorOf(compactedList, kOtherTile), nullptr);
        EXPECT_NE(decorOf(compactedList, kMossTile), nullptr);
    }

    TEST(DecorTest, SolveDecor_DressesTheTopsItsBasesAllow)
    {
        const auto voxels = antwika::voxel::getExpandCubesToVoxels(
            antwika::voxel::voxelsOf({antwika::voxel::VoxelCell{}}));
        const auto faces = visibleFacesOf(voxels);
        const auto drawnTiles = getDefaultTiles(faces);

        std::vector<DecorTile> decor =
            getWithDecorToggled({}, kOneTile);

        for (const auto tile : drawnTiles)
        {
            if (decorOf(decor, kOneTile)->allowedBaseTiles.empty()
                || decorOf(decor, kOneTile)->allowedBaseTiles.back() != tile)
            {
                decor = getWithBaseToggled(decor, kOneTile, tile);
            }
        }

        const auto placements =
            getSolveDecor(faces, drawnTiles, decor, TileRules{}, 0);

        EXPECT_FALSE(placements.empty());

        const auto bare = getSolveDecor(
            faces,
            drawnTiles,
            getWithDecorToggled({}, kOneTile),
            TileRules{},
            0);

        EXPECT_TRUE(bare.empty());
    }

    TEST(DecorTest, HasAnimatedDecor_SeesNoAnimationInStillTiles)
    {
        const std::vector<DecorTile> decor{
            DecorTile{.tile = kOneTile, .frameTiles = {kOneTile}},
            DecorTile{.tile = kOtherTile, .frameTiles = {}}};

        EXPECT_FALSE(hasAnimatedDecor(decor));
    }

    TEST(DecorTest, HasAnimatedDecor_SeesATileHoldingSeveralFrames)
    {
        const std::vector<DecorTile> decor{
            DecorTile{.tile = kOneTile, .frameTiles = {kOneTile}},
            DecorTile{
                .tile = kOtherTile,
                .frameTiles = {kOtherTile, kOneTile}}};

        EXPECT_TRUE(hasAnimatedDecor(decor));
    }

    TEST(DecorTest, FrameWidget_TellsOneFrameFromTheNext)
    {
        EXPECT_NE(getFrameWidget(0), getFrameWidget(1));
        EXPECT_EQ(
            static_cast<std::uint64_t>(getFrameWidget(3))
                - static_cast<std::uint64_t>(getFrameWidget(0)),
            3U);
        EXPECT_NE(getFrameWidget(0), antwika::decor::kFrameAddWidget);
    }

    TEST(DecorTest, MemberWidget_TellsOneMemberFromTheNext)
    {
        EXPECT_NE(getMemberWidget(0), getMemberWidget(1));
        EXPECT_EQ(
            static_cast<std::uint64_t>(getMemberWidget(2))
                - static_cast<std::uint64_t>(getMemberWidget(0)),
            2U);
        EXPECT_NE(getMemberWidget(0), getFrameWidget(0));
    }

}
