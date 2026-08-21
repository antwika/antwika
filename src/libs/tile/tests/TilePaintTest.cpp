#include <gtest/gtest.h>

#include <set>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include <antwika/tile/TilePaint.hpp>

using antwika::tilemap::Atlas;
using antwika::tilemap::atlasSize;
using antwika::tilemap::kFloorTileSize;
using antwika::tile::kPaletteColors;
using antwika::tile::kPaletteSize;
using antwika::tilemap::kWallTileSize;
using antwika::tile::paint;
using antwika::tile::paintedAt;
using antwika::tile::filledPixels;
using antwika::tile::linePixels;
using antwika::tile::paintFill;
using antwika::tile::paintLine;
using antwika::tile::pixelAt;
using antwika::tile::pixelPlace;
using antwika::tilemap::Tile;
using antwika::tilemap::tilePixels;
using antwika::tilemap::tileSizeOf;
using antwika::geometry::GridCell;
using antwika::gfx::PointF;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::RectF;
using antwika::gfx::Size;

namespace
{
    constexpr float kDrawnScale = 6.0F;

    [[nodiscard]] RectF inspectedTileRect(const Tile tile)
    {
        const auto size = tileSizeOf(tile.atlas);

        return RectF(
            PointF{20.0F, 30.0F},
            antwika::geometry::SizeF{
                static_cast<float>(size.width) * kDrawnScale,
                static_cast<float>(size.height) * kDrawnScale});
    }

    constexpr Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    [[nodiscard]] Bitmap blankAtlas(const Atlas atlas)
    {
        const auto wholeSize = atlasSize(tileSizeOf(atlas));

        return Bitmap{
            .size = wholeSize,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(wholeSize.width)
                    * wholeSize.height * antwika::gfx::kBytesPerPixel,
                0)};
    }

    [[nodiscard]] PointF middleOf(const RectF whereRect)
    {
        return PointF{
            whereRect.originPoint.x + (whereRect.size.width / 2.0F),
            whereRect.originPoint.y + (whereRect.size.height / 2.0F)};
    }
}






TEST(TilePaintTest, PixelAt_FindsTheCornersOfATile)
{
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 0};
    const auto where = inspectedTileRect(tile);
    const auto tileSize = tileSizeOf(tile.atlas);
    const auto first = pixelAt(
        tile,
        where,
        PointF{where.originPoint.x + 0.1F, where.originPoint.y + 0.1F});
    const auto lastColor = pixelAt(
        tile,
        where,
        PointF{
            where.originPoint.x + where.size.width - 0.1F,
            where.originPoint.y + where.size.height - 0.1F});

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(lastColor.has_value());
    EXPECT_EQ(first->column, 0U);
    EXPECT_EQ(first->row, 0U);
    EXPECT_EQ(lastColor->column, tileSize.width - 1);
    EXPECT_EQ(lastColor->row, tileSize.height - 1);
}

TEST(TilePaintTest, PixelAt_FindsNothingOutsideTheTile)
{
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 0};
    const auto where = inspectedTileRect(tile);

    EXPECT_FALSE(
        pixelAt(
            tile,
            where,
            PointF{where.originPoint.x - 1.0F, where.originPoint.y})
            .has_value());
    EXPECT_FALSE(
        pixelAt(
            tile,
            where,
            PointF{
                where.originPoint.x,
                where.originPoint.y + where.size.height + 1.0F})
            .has_value());
}

TEST(TilePaintTest, PixelAt_ReachesEveryPixelOfATile)
{
    constexpr Tile tile{.atlas = Atlas::Wall, .index = 0};
    const auto where = inspectedTileRect(tile);
    const auto tileSize = tileSizeOf(tile.atlas);
    std::set<std::pair<std::uint32_t, std::uint32_t>> reachedCells;

    for (auto downPoint = where.originPoint.y;
         downPoint < where.originPoint.y + where.size.height;
         downPoint += 0.25F)
    {
        for (auto acrossPoint = where.originPoint.x;
             acrossPoint < where.originPoint.x + where.size.width;
             acrossPoint += 0.25F)
        {
            const auto pixel =
                pixelAt(tile, where, PointF{acrossPoint, downPoint});

            if (pixel.has_value())
            {
                reachedCells.insert({pixel->column, pixel->row});
            }
        }
    }

    EXPECT_EQ(
        reachedCells.size(),
        static_cast<std::size_t>(tileSize.width)
            * tileSize.height);
}

TEST(TilePaintTest, Paint_ColorsThePixelItIsGiven)
{
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 37};
    auto atlas = blankAtlas(Atlas::Floor);
    constexpr GridCell pixelCell{.column = 3, .row = 5};

    antwika::tile::paint(atlas, tile, pixelCell, kRedColor);

    EXPECT_EQ(paintedAt(atlas, tile, pixelCell), kRedColor);
}

TEST(TilePaintTest, Paint_LeavesTheRestOfTheAtlasAlone)
{
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 37};
    auto atlas = blankAtlas(Atlas::Floor);
    const auto beforePixels = atlas.pixels;

    antwika::tile::paint(atlas, tile, {.column = 3, .row = 5}, kRedColor);

    auto changedCount = 0;

    for (std::size_t index = 0; index < atlas.pixels.size(); ++index)
    {
        changedCount += atlas.pixels[index] != beforePixels[index] ? 1 : 0;
    }

    EXPECT_LE(changedCount, static_cast<int>(antwika::gfx::kBytesPerPixel));
}

TEST(TilePaintTest, Paint_WritesWithinTheTileItIsGiven)
{
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 37};
    auto atlas = blankAtlas(Atlas::Floor);
    const auto tileRect = tilePixels(tile.index, kFloorTileSize);

    antwika::tile::paint(atlas, tile, {.column = 0, .row = 0}, kRedColor);

    const auto byteIndex =
        ((static_cast<std::size_t>(tileRect.originPoint.y)
          * atlas.size.width)
         + static_cast<std::size_t>(tileRect.originPoint.x))
        * antwika::gfx::kBytesPerPixel;

    EXPECT_EQ(atlas.pixels[byteIndex], kRedColor.red);
}

TEST(TilePaintTest, Paint_TellsOneTileOfAnAtlasFromTheNext)
{
    constexpr Tile oneTile{.atlas = Atlas::Wall, .index = 10};
    constexpr Tile otherTile{.atlas = Atlas::Wall, .index = 11};
    auto atlas = blankAtlas(Atlas::Wall);
    constexpr GridCell pixelCell{.column = 2, .row = 2};

    antwika::tile::paint(atlas, oneTile, pixelCell, kRedColor);

    EXPECT_EQ(paintedAt(atlas, oneTile, pixelCell), kRedColor);
    EXPECT_NE(paintedAt(atlas, otherTile, pixelCell), kRedColor);
}

TEST(TilePaintTest, PixelPlace_StandsWherePixelAtWouldFindIt)
{
    const Tile tile{.atlas = Atlas::Floor, .index = 3};
    const auto where = inspectedTileRect(tile);

    for (const auto pixelCell :
         {GridCell{.column = 0, .row = 0},
          GridCell{.column = 5, .row = 7},
          GridCell{
              .column = tileSizeOf(tile.atlas).width - 1,
              .row = tileSizeOf(tile.atlas).height - 1}})
    {
        const auto place = pixelPlace(tile, where, pixelCell);

        EXPECT_EQ(pixelAt(tile, where, middleOf(place)), pixelCell);
    }
}

TEST(TilePaintTest, PixelPlace_KeepsEveryPixelInsideTheTile)
{
    const Tile tile{.atlas = Atlas::Wall, .index = 1};
    const auto where = inspectedTileRect(tile);
    const auto tileSize = tileSizeOf(tile.atlas);

    for (std::uint32_t row = 0; row < tileSize.height; ++row)
    {
        for (std::uint32_t column = 0; column < tileSize.width;
             ++column)
        {
            const auto place = pixelPlace(
                tile, where, GridCell{.column = column, .row = row});

            EXPECT_GE(place.originPoint.x, where.originPoint.x - 1e-3F);
            EXPECT_GE(place.originPoint.y, where.originPoint.y - 1e-3F);
            EXPECT_LE(
                place.originPoint.x + place.size.width,
                where.originPoint.x + where.size.width + 1e-3F);
            EXPECT_LE(
                place.originPoint.y + place.size.height,
                where.originPoint.y + where.size.height + 1e-3F);
        }
    }
}

TEST(TilePaintTest, LinePixels_RunsFromTheOneToTheOther)
{
    const auto run = linePixels(
        GridCell{.column = 2, .row = 3},
        GridCell{.column = 9, .row = 6});

    ASSERT_FALSE(run.empty());
    EXPECT_EQ(run.front(), (GridCell{.column = 2, .row = 3}));
    EXPECT_EQ(run.back(), (GridCell{.column = 9, .row = 6}));
}

TEST(TilePaintTest, LinePixels_LeavesNoGapBetweenPixels)
{
    const auto run = linePixels(
        GridCell{.column = 1, .row = 1},
        GridCell{.column = 12, .row = 4});

    for (std::size_t step = 1; step < run.size(); ++step)
    {
        const auto hereCell = run.at(step);
        const auto lastCell = run.at(step - 1);
        const auto columnStep =
            std::abs(
                static_cast<std::int64_t>(hereCell.column)
                - static_cast<std::int64_t>(lastCell.column));
        const auto rowStep =
            std::abs(
                static_cast<std::int64_t>(hereCell.row)
                - static_cast<std::int64_t>(lastCell.row));

        EXPECT_LE(columnStep, 1);
        EXPECT_LE(rowStep, 1);
        EXPECT_GT(columnStep + rowStep, 0);
    }
}

TEST(TilePaintTest, LinePixels_GivesOnePixelForALineOfNoLength)
{
    const GridCell cell{.column = 4, .row = 5};

    EXPECT_EQ(linePixels(cell, cell).size(), 1U);
}

TEST(TilePaintTest, PaintLine_ColorsEveryPixelOfTheRun)
{
    auto atlas = blankAtlas(Atlas::Floor);
    const Tile tile{.atlas = Atlas::Floor, .index = 2};
    const GridCell fromCell{.column = 1, .row = 1};
    const GridCell toCell{.column = 8, .row = 5};

    paintLine(atlas, tile, fromCell, toCell, kRedColor);

    for (const auto pixelCell : linePixels(fromCell, toCell))
    {
        EXPECT_EQ(paintedAt(atlas, tile, pixelCell), kRedColor);
    }

    EXPECT_NE(
        paintedAt(atlas, tile, GridCell{.column = 0, .row = 9}),
        kRedColor);
}

TEST(TilePaintTest, FilledPixels_TakesTheWholeOfABlankTile)
{
    auto atlas = blankAtlas(Atlas::Wall);
    const Tile tile{.atlas = Atlas::Wall, .index = 0};
    const auto tileSize = tileSizeOf(tile.atlas);
    const auto field =
        filledPixels(atlas, tile, GridCell{.column = 0, .row = 0});

    EXPECT_EQ(
        field.size(),
        static_cast<std::size_t>(tileSize.width)
            * tileSize.height);
}

TEST(TilePaintTest, FilledPixels_StopsAtAColorOfItsOwn)
{
    auto atlas = blankAtlas(Atlas::Floor);
    const Tile tile{.atlas = Atlas::Floor, .index = 0};
    const auto tileSize = tileSizeOf(tile.atlas);

    for (std::uint32_t row = 0; row < tileSize.height; ++row)
    {
        antwika::tile::paint(
            atlas, tile, GridCell{.column = 4, .row = row}, kRedColor);
    }

    const auto field =
        filledPixels(atlas, tile, GridCell{.column = 0, .row = 0});

    for (const auto pixelCell : field)
    {
        EXPECT_LT(pixelCell.column, 4U);
    }

    EXPECT_EQ(
        field.size(),
        static_cast<std::size_t>(4) * tileSize.height);
}

TEST(TilePaintTest, PaintFill_ColorsOneFieldAndLeavesTheOther)
{
    auto atlas = blankAtlas(Atlas::Floor);
    const Tile tile{.atlas = Atlas::Floor, .index = 5};
    const auto tileSize = tileSizeOf(tile.atlas);
    const Color kWallColor{
        .red = 0, .green = 0, .blue = 255, .alpha = 255};

    for (std::uint32_t row = 0; row < tileSize.height; ++row)
    {
        antwika::tile::paint(
            atlas, tile, GridCell{.column = 4, .row = row}, kWallColor);
    }

    paintFill(atlas, tile, GridCell{.column = 0, .row = 0}, kRedColor);

    EXPECT_EQ(
        paintedAt(atlas, tile, GridCell{.column = 3, .row = 2}),
        kRedColor);
    EXPECT_EQ(
        paintedAt(atlas, tile, GridCell{.column = 4, .row = 2}),
        kWallColor);
    EXPECT_NE(
        paintedAt(atlas, tile, GridCell{.column = 5, .row = 2}),
        kRedColor);
}

TEST(TilePaintTest, PaintFill_LeavesTheNeighbouringTileAlone)
{
    auto atlas = blankAtlas(Atlas::Floor);
    const Tile tile{.atlas = Atlas::Floor, .index = 0};
    const Tile nextTile{.atlas = Atlas::Floor, .index = 1};

    paintFill(atlas, tile, GridCell{.column = 0, .row = 0}, kRedColor);

    EXPECT_NE(
        paintedAt(atlas, nextTile, GridCell{.column = 0, .row = 0}),
        kRedColor);
}

TEST(TilePaintTest, SwatchWidget_GivesEveryInkASwatchOfItsOwn)
{
    std::set<antwika::ui::WidgetId> seenWidgets;

    for (std::size_t which = 0; which < antwika::tile::kMaxInks;
         ++which)
    {
        EXPECT_TRUE(
            seenWidgets.insert(antwika::tile::swatchWidget(which)).second);
    }
}

TEST(TilePaintTest, SoleInk_KnowsAnInkAloneInItsColor)
{
    const std::vector<Color> paletteColors{
        kPaletteColors.begin(), kPaletteColors.end()};

    for (std::size_t which = 0; which < paletteColors.size(); ++which)
    {
        EXPECT_TRUE(antwika::tile::soleInk(paletteColors, which));
    }
}

TEST(TilePaintTest, SoleInk_KnowsAColorTwoInksOffer)
{
    std::vector<Color> paletteColors{
        kPaletteColors.begin(), kPaletteColors.end()};

    paletteColors.push_back(paletteColors.front());

    EXPECT_FALSE(antwika::tile::soleInk(paletteColors, 0));
    EXPECT_FALSE(
        antwika::tile::soleInk(paletteColors, paletteColors.size() - 1));
    EXPECT_TRUE(antwika::tile::soleInk(paletteColors, 1));
}

TEST(TilePaintTest, PaintedWith_FindsEveryPixelOfAColor)
{
    auto atlas = blankAtlas(Atlas::Floor);
    const Tile tile{.atlas = Atlas::Floor, .index = 0};

    antwika::tile::paint(
        atlas,
        tile,
        GridCell{.column = 1, .row = 2},
        kRedColor);
    antwika::tile::paint(
        atlas,
        tile,
        GridCell{.column = 3, .row = 1},
        kRedColor);
    antwika::tile::paint(
        atlas, tile, GridCell{.column = 2, .row = 2}, kPaletteColors[2]);

    EXPECT_EQ(antwika::tile::paintedWith(atlas, kRedColor).size(), 2U);
    EXPECT_EQ(
        antwika::tile::paintedWith(atlas, kPaletteColors[2]).size(), 1U);
}

TEST(TilePaintTest, PaintedWith_LeavesAPixelRubbedOutAlone)
{
    const auto atlas = blankAtlas(Atlas::Floor);

    EXPECT_TRUE(
        antwika::tile::paintedWith(atlas, Color{.alpha = 0})
            .empty());
}

TEST(TilePaintTest, RepaintAt_CarriesEveryPixelToTheColor)
{
    auto atlas = blankAtlas(
        Atlas::Floor);
    const Tile tile{.atlas = Atlas::Floor, .index = 0};
    const auto otherTile = kPaletteColors[3];

    antwika::tile::paint(
        atlas,
        tile,
        GridCell{.column = 1, .row = 2},
        kRedColor);
    antwika::tile::paint(
        atlas,
        tile,
        GridCell{.column = 3, .row = 1},
        kRedColor);
    antwika::tile::paint(
        atlas,
        tile,
        GridCell{.column = 2, .row = 2},
        otherTile);

    antwika::tile::repaintAt(
        atlas,
        antwika::tile::paintedWith(atlas, kRedColor),
        kPaletteColors[4]);

    EXPECT_EQ(
        paintedAt(atlas, tile, GridCell{.column = 1, .row = 2}),
        kPaletteColors[4]);
    EXPECT_EQ(
        paintedAt(atlas, tile, GridCell{.column = 3, .row = 1}),
        kPaletteColors[4]);
    EXPECT_EQ(
        paintedAt(atlas, tile, GridCell{.column = 2, .row = 2}),
        otherTile);
    EXPECT_TRUE(antwika::tile::paintedWith(atlas, kRedColor).empty());
}
