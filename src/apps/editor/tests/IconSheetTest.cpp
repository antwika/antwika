#include <gtest/gtest.h>

#include <antwika/gfx/SizeF.hpp>

#include "antwika/editor/ui/IconSheet.hpp"

using antwika::editor::getEditedIconRect;
using antwika::editor::getIconDrawBounds;
using antwika::editor::getIconSheetBounds;
using antwika::editor::iconCellAt;
using antwika::editor::getIconCellRect;
using antwika::editor::getIconCount;
using antwika::editor::getIconPixelColor;
using antwika::editor::iconPixelAt;
using antwika::editor::getIconPixelRect;
using antwika::editor::getIconSource;
using antwika::editor::kIconCellSize;
using antwika::editor::kIconColumns;
using antwika::editor::setIconPixel;

namespace
{
    constexpr antwika::gfx::Size kCanvasSize{480, 270};

    constexpr std::size_t kSomeIcons = 31;

    [[nodiscard]] antwika::gfx::Bitmap getBlankSheet(
        const std::size_t count)
    {
        return antwika::gfx::Bitmap{
            .size =
                {.width = static_cast<std::uint32_t>(
                     count * kIconCellSize.width),
                 .height = kIconCellSize.height},
            .pixels = std::vector<std::uint8_t>(
                count * kIconCellSize.width * kIconCellSize.height
                    * 4,
                0)};
    }
}

TEST(IconSheetTest, IconCount_CountsOneIconToACell)
{
    EXPECT_EQ(getIconCount(getBlankSheet(kSomeIcons).size), kSomeIcons);
}

TEST(IconSheetTest, IconSource_CutsEachIconFromItsOwnCell)
{
    const auto first = getIconSource(0);
    const auto second = getIconSource(1);

    EXPECT_EQ(first.originPoint.x, 0);
    EXPECT_EQ(
        second.originPoint.x,
        static_cast<std::int32_t>(kIconCellSize.width));
    EXPECT_EQ(first.size, kIconCellSize);
}

TEST(IconSheetTest, IconCellRect_LaysTheIconsSoManyToARow)
{
    const auto sheetRect = getIconSheetBounds(kCanvasSize);
    const auto first = getIconCellRect(sheetRect, kSomeIcons, 0);
    const auto besideRect = getIconCellRect(sheetRect, kSomeIcons, 1);
    const auto underRect =
        getIconCellRect(sheetRect, kSomeIcons, kIconColumns);

    EXPECT_GT(besideRect.originPoint.x, first.originPoint.x);
    EXPECT_EQ(besideRect.originPoint.y, first.originPoint.y);
    EXPECT_EQ(underRect.originPoint.x, first.originPoint.x);
    EXPECT_GT(underRect.originPoint.y, first.originPoint.y);
}

TEST(IconSheetTest, IconCellAt_FindsTheCellUnderItsOwnMiddle)
{
    for (const std::size_t index : {0UL, 5UL, 12UL, 30UL})
    {
        const auto place = getIconCellRect(
            getIconSheetBounds(kCanvasSize), kSomeIcons, index);

        EXPECT_EQ(
            iconCellAt(
                getIconSheetBounds(kCanvasSize),
                kSomeIcons,
                {place.originPoint.x + (place.size.width / 2.0F),
                 place.originPoint.y
                     + (place.size.height / 2.0F)}),
            index);
    }
}

TEST(IconSheetTest, IconCellRect_KeepsEveryCellInsideTheSheetRect)
{
    const antwika::gfx::RectF sheetRect(
        antwika::gfx::PointF{40.0F, 30.0F},
        antwika::gfx::SizeF{90.0F, 200.0F});

    for (std::size_t index = 0; index < kSomeIcons; ++index)
    {
        const auto place = getIconCellRect(sheetRect, kSomeIcons, index);

        EXPECT_GE(place.originPoint.x, sheetRect.originPoint.x - 0.01F);
        EXPECT_GE(place.originPoint.y, sheetRect.originPoint.y - 0.01F);
        EXPECT_LE(
            place.originPoint.x + place.size.width,
            sheetRect.originPoint.x + sheetRect.size.width + 0.01F);
        EXPECT_LE(
            place.originPoint.y + place.size.height,
            sheetRect.originPoint.y + sheetRect.size.height + 0.01F);
    }
}

TEST(IconSheetTest, IconCellAt_FindsNothingBesideTheSheetRect)
{
    const antwika::gfx::RectF sheetRect(
        antwika::gfx::PointF{40.0F, 30.0F},
        antwika::gfx::SizeF{90.0F, 200.0F});

    for (const antwika::gfx::PointF point :
         {antwika::gfx::PointF{300.0F, 100.0F},
          antwika::gfx::PointF{10.0F, 100.0F},
          antwika::gfx::PointF{80.0F, 250.0F},
          antwika::gfx::PointF{80.0F, 10.0F}})
    {
        EXPECT_FALSE(iconCellAt(sheetRect, kSomeIcons, point).has_value());
    }
}

TEST(IconSheetTest, IconCellRect_KeepsACellWhereNoIconIsCounted)
{
    const antwika::gfx::RectF sheetRect(
        antwika::gfx::PointF{40.0F, 30.0F},
        antwika::gfx::SizeF{90.0F, 200.0F});

    EXPECT_GT(getIconCellRect(sheetRect, 0, 0).size.width, 0.0F);
}

TEST(IconSheetTest, IconCellAt_FindsNothingInTheGapBetweenTwoCells)
{
    const auto sheetRect = getIconSheetBounds(kCanvasSize);
    const auto first = getIconCellRect(sheetRect, kSomeIcons, 0);
    const auto nextCell = getIconCellRect(sheetRect, kSomeIcons, 1);
    const auto gapMiddle =
        (first.originPoint.x + first.size.width + nextCell.originPoint.x)
        / 2.0F;

    ASSERT_GT(
        nextCell.originPoint.x, first.originPoint.x + first.size.width);
    EXPECT_FALSE(
        iconCellAt(
            sheetRect,
            kSomeIcons,
            {gapMiddle,
             first.originPoint.y + (first.size.height / 2.0F)})
            .has_value());
}

TEST(IconSheetTest, EditedIconRect_StandsClearOfEveryCell)
{
    const auto drawnRect =
        getEditedIconRect(getIconDrawBounds(kCanvasSize));

    for (std::size_t index = 0; index < kSomeIcons; ++index)
    {
        const auto place = getIconCellRect(
            getIconSheetBounds(kCanvasSize), kSomeIcons, index);

        EXPECT_LE(
            place.originPoint.x + place.size.width,
            drawnRect.originPoint.x);
    }
}

TEST(IconSheetTest, IconPixelAt_FindsEveryPixelUnderItsPlace)
{
    const auto room =
        getEditedIconRect(getIconDrawBounds(kCanvasSize));

    for (const std::uint32_t column : {0U, 7U, 15U})
    {
        for (const std::uint32_t row : {0U, 8U, 15U})
        {
            const antwika::geometry::GridCell pixelCell{
                column, row};
            const auto place = getIconPixelRect(room, pixelCell);

            EXPECT_EQ(
                iconPixelAt(
                    room,
                    {place.originPoint.x
                         + (place.size.width / 2.0F),
                     place.originPoint.y
                         + (place.size.height / 2.0F)}),
                pixelCell);
        }
    }
}

TEST(IconSheetTest, IconPixelAt_FindsNothingOutsideTheIcon)
{
    const auto room =
        getEditedIconRect(getIconDrawBounds(kCanvasSize));

    EXPECT_FALSE(
        iconPixelAt(
            room,
            {room.originPoint.x - 1.0F, room.originPoint.y})
            .has_value());
}

TEST(IconSheetTest, SetIconPixel_SetsThePixelOfThatIconAlone)
{
    auto sheet = getBlankSheet(3);
    constexpr antwika::gfx::Color kTextColor{
        .red = 255, .green = 255, .blue = 255, .alpha = 255};

    setIconPixel(sheet, 1, {2, 3}, kTextColor);

    EXPECT_EQ(getIconPixelColor(sheet, 1, {2, 3}), kTextColor);
    EXPECT_EQ(getIconPixelColor(sheet, 0, {2, 3}).alpha, 0);
    EXPECT_EQ(getIconPixelColor(sheet, 2, {2, 3}).alpha, 0);
}

TEST(IconSheetTest, IconCellRect_FollowsTheSheetRectItIsGiven)
{
    const auto restingRect = getIconSheetBounds(kCanvasSize);
    const antwika::gfx::RectF movedRect(
        antwika::gfx::PointF{
            restingRect.originPoint.x + 40.0F,
            restingRect.originPoint.y},
        restingRect.size);

    const auto restingCell = getIconCellRect(restingRect, kSomeIcons, 0);
    const auto movedCell = getIconCellRect(movedRect, kSomeIcons, 0);

    EXPECT_FLOAT_EQ(
        movedCell.originPoint.x, restingCell.originPoint.x + 40.0F);
    EXPECT_FLOAT_EQ(movedCell.originPoint.y, restingCell.originPoint.y);
}

TEST(IconSheetTest, EditedIconRect_HangsFromTheTopOfTheDrawingRect)
{
    const antwika::gfx::RectF drawRect(
        antwika::gfx::PointF{100.0F, 20.0F},
        antwika::gfx::SizeF{200.0F, 400.0F});
    const auto drawnRect = getEditedIconRect(drawRect);

    EXPECT_GT(drawnRect.originPoint.y, drawRect.originPoint.y);
    EXPECT_LT(
        drawnRect.originPoint.y,
        drawRect.originPoint.y + (drawRect.size.height / 2.0F));
}

TEST(IconSheetTest, EditedIconRect_StandsAgainstTheRightOfTheDrawingRect)
{
    for (const float wide : {80.0F, 160.0F, 400.0F})
    {
        const antwika::gfx::RectF drawRect(
            antwika::gfx::PointF{100.0F, 20.0F},
            antwika::gfx::SizeF{wide, 200.0F});
        const auto drawnRect = getEditedIconRect(drawRect);

        EXPECT_FLOAT_EQ(
            drawnRect.originPoint.x + drawnRect.size.width,
            drawRect.originPoint.x + drawRect.size.width);
    }
}

TEST(IconSheetTest, EditedIconRect_TakesTheWidthOfATallDrawingRect)
{
    const antwika::gfx::RectF drawRect(
        antwika::gfx::PointF{0.0F, 0.0F},
        antwika::gfx::SizeF{40.0F, 400.0F});
    const auto drawnRect = getEditedIconRect(drawRect);

    EXPECT_FLOAT_EQ(drawnRect.size.width, drawnRect.size.height);
    EXPECT_FLOAT_EQ(drawnRect.size.width, drawRect.size.width);
}

TEST(IconSheetTest, EditedIconRect_TakesTheHeightOfAWideDrawingRect)
{
    const antwika::gfx::RectF drawRect(
        antwika::gfx::PointF{0.0F, 0.0F},
        antwika::gfx::SizeF{400.0F, 80.0F});
    const auto drawnRect = getEditedIconRect(drawRect);

    EXPECT_FLOAT_EQ(drawnRect.size.width, drawnRect.size.height);
    EXPECT_LT(drawnRect.size.width, drawRect.size.width);
    EXPECT_LE(
        drawnRect.originPoint.y + drawnRect.size.height,
        drawRect.originPoint.y + drawRect.size.height);
}

TEST(IconSheetTest, EditedIconRect_KeepsNoRoomInsideAnEmptyDrawingRect)
{
    const auto drawnRect = getEditedIconRect(
        antwika::gfx::RectF(
            antwika::gfx::PointF{0.0F, 0.0F},
            antwika::gfx::SizeF{0.0F, 0.0F}));

    EXPECT_FLOAT_EQ(drawnRect.size.width, 0.0F);
    EXPECT_FLOAT_EQ(drawnRect.size.height, 0.0F);
}

TEST(IconSheetTest, SetIconPixel_LeavesAPixelOffTheSheetAlone)
{
    auto sheet = getBlankSheet(2);
    const auto beforeSheet = sheet;

    setIconPixel(
        sheet,
        5,
        {0, 0},
        antwika::gfx::Color{.red = 255, .alpha = 255});

    EXPECT_EQ(sheet, beforeSheet);
}
