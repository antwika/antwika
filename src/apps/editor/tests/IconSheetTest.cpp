#include <gtest/gtest.h>

#include "antwika/editor/ui/IconSheet.hpp"

using antwika::editor::getEditedIconRect;
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
    const auto first = getIconCellRect(kCanvasSize, kSomeIcons, 0);
    const auto besideRect = getIconCellRect(kCanvasSize, kSomeIcons, 1);
    const auto underRect =
        getIconCellRect(kCanvasSize, kSomeIcons, kIconColumns);

    EXPECT_GT(besideRect.originPoint.x, first.originPoint.x);
    EXPECT_EQ(besideRect.originPoint.y, first.originPoint.y);
    EXPECT_EQ(underRect.originPoint.x, first.originPoint.x);
    EXPECT_GT(underRect.originPoint.y, first.originPoint.y);
}

TEST(IconSheetTest, IconCellAt_FindsTheCellUnderItsOwnMiddle)
{
    for (const std::size_t index : {0UL, 5UL, 12UL, 30UL})
    {
        const auto place =
            getIconCellRect(kCanvasSize, kSomeIcons, index);

        EXPECT_EQ(
            iconCellAt(
                kCanvasSize,
                kSomeIcons,
                {place.originPoint.x + (place.size.width / 2.0F),
                 place.originPoint.y
                     + (place.size.height / 2.0F)}),
            index);
    }
}

TEST(IconSheetTest, IconCellAt_FindsNothingOffEveryCell)
{
    EXPECT_FALSE(
        iconCellAt(kCanvasSize, kSomeIcons, {0.0F, 0.0F})
            .has_value());
}

TEST(IconSheetTest, EditedIconRect_StandsClearOfEveryCell)
{
    const auto drawnRect = getEditedIconRect(kCanvasSize);

    for (std::size_t index = 0; index < kSomeIcons; ++index)
    {
        const auto place =
            getIconCellRect(kCanvasSize, kSomeIcons, index);

        EXPECT_LE(
            place.originPoint.x + place.size.width,
            drawnRect.originPoint.x);
    }
}

TEST(IconSheetTest, IconPixelAt_FindsEveryPixelUnderItsPlace)
{
    const auto room = getEditedIconRect(kCanvasSize);

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
    const auto room = getEditedIconRect(kCanvasSize);

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
