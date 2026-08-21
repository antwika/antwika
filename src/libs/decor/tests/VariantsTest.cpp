#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/tile/TileRules.hpp>

#include <antwika/decor/Decor.hpp>
#include <antwika/decor/Variants.hpp>

using antwika::tilemap::Atlas;
using antwika::voxel::Corner;
using antwika::decor::DecorTile;
using antwika::voxelmap::FaceRef;
using antwika::decor::groupContaining;
using antwika::decor::groupLedBy;
using antwika::decor::kFullFrequency;
using antwika::tilemap::kEveryTileEdge;
using antwika::voxel::Kind;
using antwika::decor::canonicalTileOf;
using antwika::decor::canBeVariantOf;
using antwika::tilemap::Tile;
using antwika::tile::TileRules;
using antwika::decor::VariantGroup;
using antwika::decor::VariantMember;
using antwika::decor::withVariantsApplied;
using antwika::voxel::VoxelCell;
using antwika::decor::withVariantToggled;
using antwika::decor::withVariantWeightSet;

namespace
{
    [[nodiscard]] Tile flat(const std::uint16_t index)
    {
        return Tile{.atlas = Atlas::Floor, .index = index};
    }

    [[nodiscard]] std::vector<VariantGroup> aFamily()
    {
        return {
            VariantGroup{
                .canonicalTile = flat(1),
                .variants = {
                    VariantMember{.tile = flat(2)},
                    VariantMember{.tile = flat(3)}}}};
    }

    [[nodiscard]] std::vector<FaceRef> buildableTopOutlines(
        const std::size_t many)
    {
        std::vector<FaceRef> faces;

        for (std::size_t tileIndex = 0; tileIndex < many; ++tileIndex)
        {
            faces.push_back(
                FaceRef{
                    .cell =
                        VoxelCell{
                            .x = static_cast<std::int32_t>(
                                tileIndex % 64),
                            .y = 0,
                            .z = static_cast<std::int32_t>(
                                tileIndex / 64)},
                    .side = 4});
        }

        return faces;
    }
}

TEST(VariantsTest, GroupLedBy_FindsTheCanonicalTileAlone)
{
    const auto families = aFamily();

    EXPECT_NE(groupLedBy(families, flat(1)), nullptr);
    EXPECT_EQ(groupLedBy(families, flat(2)), nullptr);
    EXPECT_EQ(groupLedBy(families, flat(9)), nullptr);
}

TEST(VariantsTest, GroupContaining_FindsTheVariantsAlone)
{
    const auto families = aFamily();

    EXPECT_EQ(groupContaining(families, flat(1)), nullptr);
    EXPECT_NE(groupContaining(families, flat(2)), nullptr);
    EXPECT_NE(groupContaining(families, flat(3)), nullptr);
    EXPECT_EQ(groupContaining(families, flat(9)), nullptr);
}

TEST(VariantsTest, CanonicalTileOf_CarriesAVariantToItsCanonicalTile)
{
    const auto families = aFamily();

    EXPECT_EQ(canonicalTileOf(families, flat(2)), flat(1));
    EXPECT_EQ(canonicalTileOf(families, flat(1)), flat(1));
    EXPECT_EQ(canonicalTileOf(families, flat(9)), flat(9));
}

TEST(VariantsTest, WithVariantToggled_BeginsAGroupWithItsFirst)
{
    const auto families =
        withVariantToggled({}, flat(1), flat(2));

    ASSERT_EQ(families.size(), 1U);
    EXPECT_EQ(families.at(0).canonicalTile, flat(1));
    ASSERT_EQ(families.at(0).variants.size(), 1U);
    EXPECT_EQ(families.at(0).variants.at(0).tile, flat(2));
}

TEST(VariantsTest, WithVariantToggled_EndsAGroupWithItsLast)
{
    auto families = withVariantToggled({}, flat(1), flat(2));

    families = withVariantToggled(families, flat(1), flat(2));

    EXPECT_TRUE(families.empty());
}

TEST(VariantsTest, WithVariantToggled_LetsOneVariantGoOfSeveral)
{
    const auto families =
        withVariantToggled(aFamily(), flat(1), flat(2));

    ASSERT_EQ(families.size(), 1U);
    ASSERT_EQ(families.at(0).variants.size(), 1U);
    EXPECT_EQ(families.at(0).variants.at(0).tile, flat(3));
}

TEST(VariantsTest, WithVariantToggled_LeavesACanonicalTileOutOfItself)
{
    EXPECT_TRUE(
        withVariantToggled({}, flat(1), flat(1)).empty());
}

TEST(VariantsTest, WithVariantWeightSet_SaysAVariantsAfresh)
{
    const auto families =
        withVariantWeightSet(aFamily(), flat(3), 20);

    EXPECT_EQ(families.at(0).variants.at(1).weight, 20);
    EXPECT_EQ(families.at(0).variants.at(0).weight, kFullFrequency);
}

TEST(VariantsTest, WithVariantWeightSet_SaysTheCanonicalTilesAfresh)
{
    const auto families =
        withVariantWeightSet(aFamily(), flat(1), 5);

    EXPECT_EQ(families.at(0).weight, 5);
}

TEST(VariantsTest, WithVariantWeightSet_HoldsAWeightToFullFrequency)
{
    const auto families =
        withVariantWeightSet(aFamily(), flat(2), 255);

    EXPECT_EQ(families.at(0).variants.at(0).weight, kFullFrequency);
}

TEST(VariantsTest, WithVariantWeightSet_LeavesAStrangerUnsaid)
{
    EXPECT_EQ(
        withVariantWeightSet(aFamily(), flat(9), 5), aFamily());
}

TEST(VariantsTest, CanBeVariantOf_TakesARulelessTileOfTheAtlas)
{
    EXPECT_TRUE(
        canBeVariantOf({}, TileRules{}, {}, flat(1), flat(2)));
}

TEST(VariantsTest, CanBeVariantOf_RefusesTheCanonicalTileItself)
{
    EXPECT_FALSE(
        canBeVariantOf({}, TileRules{}, {}, flat(1), flat(1)));
}

TEST(VariantsTest, CanBeVariantOf_RefusesTheOtherAtlas)
{
    EXPECT_FALSE(
        canBeVariantOf(
            {},
            TileRules{},
            {},
            flat(1),
            Tile{.atlas = Atlas::Wall, .index = 2}));
}

TEST(VariantsTest, CanBeVariantOf_RefusesASpokenOfTile)
{
    TileRules rules;

    rules.allow(flat(2), kEveryTileEdge.at(0), flat(1));

    EXPECT_FALSE(
        canBeVariantOf({}, rules, {}, flat(1), flat(2)));
}

TEST(VariantsTest, CanBeVariantOf_RefusesACorneredTile)
{
    TileRules rules;

    rules.setCorner(flat(2), Corner::TopLeft, true);

    EXPECT_FALSE(
        canBeVariantOf({}, rules, {}, flat(1), flat(2)));
}

TEST(VariantsTest, CanBeVariantOf_RefusesAKindedTile)
{
    TileRules rules;

    rules.setKind(flat(2), Kind::Water);

    EXPECT_FALSE(
        canBeVariantOf({}, rules, {}, flat(1), flat(2)));
}

TEST(VariantsTest, CanBeVariantOf_RefusesADecorTile)
{
    const std::vector<DecorTile> decor{
        DecorTile{.tile = flat(2)}};

    EXPECT_FALSE(
        canBeVariantOf({}, TileRules{}, decor, flat(1), flat(2)));
}

TEST(VariantsTest, CanBeVariantOf_RefusesATileOfAnotherGroup)
{
    EXPECT_FALSE(
        canBeVariantOf(
            aFamily(), TileRules{}, {}, flat(9), flat(2)));
}

TEST(VariantsTest, CanBeVariantOf_RefusesACanonicalTileOfAnotherGroup)
{
    EXPECT_FALSE(
        canBeVariantOf(
            aFamily(), TileRules{}, {}, flat(9), flat(1)));
}

TEST(VariantsTest, CanBeVariantOf_RefusesAVariantAsACanonicalTile)
{
    EXPECT_FALSE(
        canBeVariantOf(
            aFamily(), TileRules{}, {}, flat(2), flat(9)));
}

TEST(VariantsTest, WithVariantsApplied_LeavesTilesOfNoGroupAlone)
{
    const auto faces = buildableTopOutlines(4);
    const std::vector<Tile> wovenTiles(4, flat(9));

    EXPECT_EQ(
        withVariantsApplied(faces, wovenTiles, aFamily(), 0), wovenTiles);
}

TEST(VariantsTest, WithVariantsApplied_LeavesEverythingWithNoGroups)
{
    const auto faces = buildableTopOutlines(4);
    const std::vector<Tile> wovenTiles(4, flat(1));

    EXPECT_EQ(withVariantsApplied(faces, wovenTiles, {}, 0), wovenTiles);
}

TEST(VariantsTest, VariantTiles_DrawsTheSameScatterTwice)
{
    const auto faces = buildableTopOutlines(256);
    const std::vector<Tile> wovenTiles(256, flat(1));
    const auto families = aFamily();

    EXPECT_EQ(
        withVariantsApplied(faces, wovenTiles, families, 0),
        withVariantsApplied(faces, wovenTiles, families, 0));
}

TEST(VariantsTest, WithVariantsApplied_ScattersEveryTileOfTheGroup)
{
    const auto faces = buildableTopOutlines(256);
    const std::vector<Tile> wovenTiles(256, flat(1));
    const auto tiles =
        withVariantsApplied(faces, wovenTiles, aFamily(), 0);
    const std::set<Tile> seenTiles(tiles.begin(), tiles.end());

    EXPECT_EQ(seenTiles.size(), 3U);
}

TEST(VariantsTest, VariantTiles_NeverDrawsAWeightlessVariant)
{
    const auto faces = buildableTopOutlines(256);
    const std::vector<Tile> wovenTiles(256, flat(1));
    const auto families =
        withVariantWeightSet(aFamily(), flat(3), 0);
    const auto tiles =
        withVariantsApplied(faces, wovenTiles, families, 0);
    const std::set<Tile> seenTiles(tiles.begin(), tiles.end());

    EXPECT_EQ(seenTiles.count(flat(3)), 0U);
    EXPECT_EQ(seenTiles.size(), 2U);
}

TEST(VariantsTest, WithVariantsApplied_KeepsACanonicalTileWeighedAlone)
{
    const auto faces = buildableTopOutlines(64);
    const std::vector<Tile> wovenTiles(64, flat(1));
    auto families = withVariantWeightSet(aFamily(), flat(2), 0);

    families = withVariantWeightSet(families, flat(3), 0);

    EXPECT_EQ(
        withVariantsApplied(faces, wovenTiles, families, 0), wovenTiles);
}

TEST(
    VariantsTest,
    WithVariantsApplied_KeepsTheCanonicalTileWithNoWeightAtAll)
{
    const auto faces = buildableTopOutlines(64);
    const std::vector<Tile> wovenTiles(64, flat(1));
    auto families = withVariantWeightSet(aFamily(), flat(1), 0);

    families = withVariantWeightSet(families, flat(2), 0);
    families = withVariantWeightSet(families, flat(3), 0);

    EXPECT_EQ(
        withVariantsApplied(faces, wovenTiles, families, 0), wovenTiles);
}

TEST(VariantsTest, VariantTiles_LandsNearAskedShares)
{
    const auto faces = buildableTopOutlines(4096);
    const std::vector<Tile> wovenTiles(4096, flat(1));
    auto families = std::vector<VariantGroup>{
        VariantGroup{
            .canonicalTile = flat(1),
            .weight = 50,
            .variants = {
                VariantMember{.tile = flat(2), .weight = 50}}}};
    const auto tiles =
        withVariantsApplied(faces, wovenTiles, families, 0);
    const auto drawnCount = static_cast<std::size_t>(
        std::count(tiles.begin(), tiles.end(), flat(2)));

    EXPECT_GT(drawnCount, 4096U * 4 / 10);
    EXPECT_LT(drawnCount, 4096U * 6 / 10);
}

TEST(VariantsTest, VariantTiles_ScattersRatherThanBands)
{
    const auto faces = buildableTopOutlines(256);
    const std::vector<Tile> wovenTiles(256, flat(1));
    const auto tiles =
        withVariantsApplied(faces, wovenTiles, aFamily(), 0);
    std::size_t changes = 0;

    for (std::size_t tileIndex = 1; tileIndex < 64; ++tileIndex)
    {
        changes += tiles.at(tileIndex) != tiles.at(tileIndex - 1) ? 1U : 0U;
    }

    EXPECT_GT(changes, 8U);
}

TEST(VariantsTest, VariantTiles_RollsTheSidesOfACellApart)
{
    const std::vector<FaceRef> faces{
        FaceRef{.cell = VoxelCell{}, .side = 0},
        FaceRef{.cell = VoxelCell{}, .side = 1},
        FaceRef{.cell = VoxelCell{}, .side = 2},
        FaceRef{.cell = VoxelCell{}, .side = 3},
        FaceRef{.cell = VoxelCell{}, .side = 4}};
    const std::vector<Tile> wovenTiles(5, flat(1));
    const auto tiles =
        withVariantsApplied(faces, wovenTiles, aFamily(), 0);
    const std::set<Tile> seenTiles(tiles.begin(), tiles.end());

    EXPECT_GT(seenTiles.size(), 1U);
}
