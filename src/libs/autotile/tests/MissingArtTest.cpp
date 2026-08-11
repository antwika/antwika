#include <gtest/gtest.h>

#include <antwika/autotile/MissingArt.hpp>
#include <antwika/autotile/TileDraw.hpp>
#include <antwika/tileset/Atlas.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>

using antwika::autotile::artMissing;
using antwika::autotile::DrawKind;
using antwika::autotile::TileDraw;
using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::tileset::atlasIndexOf;
using antwika::tileset::PixelClass;
using antwika::tileset::Tileset;

namespace
{
    /**
     * @brief A tileset of one base sprite an artist has inked.
     */
    [[nodiscard]] Tileset drawnTileset()
    {
        Tileset set;
        addSprite(set, 0).frames[0].pixels[0] = PixelClass::Ink;

        return set;
    }

    /**
     * @brief A tileset of one base sprite nobody has drawn on.
     */
    [[nodiscard]] Tileset blankTileset()
    {
        Tileset set;
        static_cast<void>(addSprite(set, 0));

        return set;
    }
}

TEST(MissingArtTest, ArtMissing_MarksBothCliffFacePieces)
{
    const auto set = drawnTileset();
    const auto index = atlasIndexOf(set);

    EXPECT_TRUE(artMissing(
        TileDraw{.kind = DrawKind::WallRim}, set, index));
    EXPECT_TRUE(artMissing(
        TileDraw{.kind = DrawKind::WallBand}, set, index));
}

TEST(MissingArtTest, ArtMissing_MarksABridgeDeck)
{
    const auto set = drawnTileset();

    EXPECT_TRUE(artMissing(
        TileDraw{.kind = DrawKind::BridgeDeck},
        set,
        atlasIndexOf(set)));
}

TEST(MissingArtTest, ArtMissing_LeavesTheLightingShadeAlone)
{
    const auto set = drawnTileset();

    EXPECT_FALSE(artMissing(
        TileDraw{.kind = DrawKind::Shade},
        set,
        atlasIndexOf(set)));
}

TEST(MissingArtTest, ArtMissing_KeepsASpriteAnArtistInked)
{
    const auto set = drawnTileset();

    EXPECT_FALSE(artMissing(
        TileDraw{.kind = DrawKind::Sprite, .atlasRow = 0},
        set,
        atlasIndexOf(set)));
}

TEST(MissingArtTest, ArtMissing_MarksASpriteNobodyDrew)
{
    const auto set = blankTileset();

    EXPECT_TRUE(artMissing(
        TileDraw{.kind = DrawKind::Sprite, .atlasRow = 0},
        set,
        atlasIndexOf(set)));
}

TEST(MissingArtTest, ArtMissing_MarksAnAnimationFrameNobodyDrew)
{
    auto set = drawnTileset();
    set.layers[0].sprites[0].frameCount = 2;

    EXPECT_FALSE(artMissing(
        TileDraw{.kind = DrawKind::Sprite, .frame = 0},
        set,
        atlasIndexOf(set)));
    EXPECT_TRUE(artMissing(
        TileDraw{.kind = DrawKind::Sprite, .frame = 1},
        set,
        atlasIndexOf(set)));
}

TEST(MissingArtTest, ArtMissing_MarksARowPastTheBakedAtlas)
{
    auto set = drawnTileset();
    const auto index = atlasIndexOf(set);

    addSprite(set, 0).frames[0].pixels[0] = PixelClass::Ink;

    EXPECT_TRUE(artMissing(
        TileDraw{.kind = DrawKind::Sprite, .atlasRow = 1},
        set,
        index));
}

TEST(MissingArtTest, ArtMissing_MarksARowTheTilesetNoLongerHolds)
{
    auto set = drawnTileset();
    addSprite(set, 0).frames[0].pixels[0] = PixelClass::Ink;

    const auto index = atlasIndexOf(set);

    set.layers[0].sprites.pop_back();

    EXPECT_TRUE(artMissing(
        TileDraw{.kind = DrawKind::Sprite, .atlasRow = 1},
        set,
        index));
}

TEST(MissingArtTest, ArtMissing_CountsRowsAcrossLayersInOrder)
{
    auto set = drawnTileset();
    static_cast<void>(addLayer(set, "decor"));
    static_cast<void>(addSprite(set, 1));
    addSprite(set, 1).frames[0].pixels[0] = PixelClass::Ink;

    const auto index = atlasIndexOf(set);

    EXPECT_TRUE(artMissing(
        TileDraw{.kind = DrawKind::Sprite, .atlasRow = 1},
        set,
        index));
    EXPECT_FALSE(artMissing(
        TileDraw{.kind = DrawKind::Sprite, .atlasRow = 2},
        set,
        index));
}
