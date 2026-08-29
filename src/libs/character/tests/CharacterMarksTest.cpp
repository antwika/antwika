#include <gtest/gtest.h>

#include <antwika/character/Character.hpp>

using antwika::character::getBlankCharacter;
using antwika::character::kCharacterCellSize;
using antwika::character::paintCharacter;

namespace
{

    [[nodiscard]] antwika::gfx::RectF getFrameRect()
    {
        return antwika::gfx::RectF(
            antwika::gfx::PointF{16.0F, 12.0F},
            antwika::geometry::SizeF{
                static_cast<float>(kCharacterCellSize.width) * 8.0F,
                static_cast<float>(kCharacterCellSize.height) * 8.0F});
    }

}

TEST(CharacterMarksTest, CopiedFrom_TakesWhatLiesBetweenTwoCorners)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{2, 3}, kRedColor);

    const auto buffer = antwika::character::copiedFrom(
        sheet,
        0,
        0,
        antwika::character::PixelSelection{
            .fromCell = antwika::geometry::GridCell{1, 2},
            .toCell = antwika::geometry::GridCell{3, 4}});

    EXPECT_EQ(buffer.size.width, 3U);
    EXPECT_EQ(buffer.size.height, 3U);
    EXPECT_EQ(buffer.pixelColors.at(4), kRedColor);
}

TEST(CharacterMarksTest, CopiedFrom_TakesTheSameEitherWayRound)
{
    const auto sheet = getBlankCharacter();
    const antwika::geometry::GridCell oneCell{1, 2};
    const antwika::geometry::GridCell otherCell{5, 7};

    EXPECT_EQ(
        antwika::character::copiedFrom(
            sheet,
            2,
            1,
            antwika::character::PixelSelection{
                .fromCell = oneCell, .toCell = otherCell}),
        antwika::character::copiedFrom(
            sheet,
            2,
            1,
            antwika::character::PixelSelection{
                .fromCell = otherCell, .toCell = oneCell}));
}

TEST(CharacterMarksTest, PasteInto_LaysABufferDownWhereItIsPut)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{0, 0}, kRedColor);

    const auto buffer = antwika::character::copiedFrom(
        sheet,
        0,
        0,
        antwika::character::PixelSelection{
            .fromCell = antwika::geometry::GridCell{0, 0},
            .toCell = antwika::geometry::GridCell{1, 1}});

    antwika::character::pasteInto(
        sheet, 1, 2, antwika::geometry::GridCell{4, 5}, buffer);

    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 1, 2, antwika::geometry::GridCell{4, 5}),
        kRedColor);
}

TEST(CharacterMarksTest, PasteInto_LeavesOffWhatWouldFallOutsideTheFrame)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    for (std::uint32_t row = 0; row < 2; ++row)
    {
        for (std::uint32_t column = 0; column < 2; ++column)
        {
            paintCharacter(
                sheet,
                0,
                0,
                antwika::geometry::GridCell{column, row},
                kRedColor);
        }
    }

    const auto buffer = antwika::character::copiedFrom(
        sheet,
        0,
        0,
        antwika::character::PixelSelection{
            .fromCell = antwika::geometry::GridCell{0, 0},
            .toCell = antwika::geometry::GridCell{1, 1}});

    antwika::character::pasteInto(
        sheet,
        0,
        0,
        antwika::geometry::GridCell{
            kCharacterCellSize.width - 1, kCharacterCellSize.height - 1},
        buffer);

    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 1, 0, antwika::geometry::GridCell{0, 0})
            .alpha,
        0);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            0,
            0,
            antwika::geometry::GridCell{
                kCharacterCellSize.width - 1,
                kCharacterCellSize.height - 1}),
        kRedColor);
}

TEST(CharacterMarksTest, SelectionRect_HoldsBothCornersItIsMarkedBy)
{
    const auto where = getFrameRect();
    const antwika::geometry::GridCell oneCell{2, 3};
    const antwika::geometry::GridCell otherCell{6, 8};
    const auto markedRect = antwika::character::getSelectionRect(
        where, antwika::character::PixelSelection{
            .fromCell = otherCell,
            .toCell = oneCell});
    const auto first =
        antwika::character::getCharacterPixelPlace(where, oneCell);
    const auto lastRect =
        antwika::character::getCharacterPixelPlace(where, otherCell);

    EXPECT_NEAR(markedRect.originPoint.x, first.originPoint.x, 0.0001F);
    EXPECT_NEAR(markedRect.originPoint.y, first.originPoint.y, 0.0001F);
    EXPECT_NEAR(
        markedRect.originPoint.x + markedRect.size.width,
        lastRect.originPoint.x + lastRect.size.width,
        0.0001F);
    EXPECT_NEAR(
        markedRect.originPoint.y + markedRect.size.height,
        lastRect.originPoint.y + lastRect.size.height,
        0.0001F);
}

TEST(CharacterMarksTest, SelectionOrigin_TakesTheSameCornerEitherWayRound)
{
    const antwika::geometry::GridCell oneCell{2, 5};
    const antwika::geometry::GridCell otherCell{6, 3};

    EXPECT_EQ(
        antwika::character::getSelectionOrigin(
            antwika::character::PixelSelection{
                .fromCell = oneCell,
                .toCell = otherCell}),
        (antwika::geometry::GridCell{2, 3}));
    EXPECT_EQ(
        antwika::character::getSelectionOrigin(
            antwika::character::PixelSelection{
                .fromCell = otherCell,
                .toCell = oneCell}),
        (antwika::geometry::GridCell{2, 3}));
}

TEST(CharacterMarksTest, SelectionSize_CountsBothPixelsItIsMarkedBy)
{
    EXPECT_EQ(
        antwika::character::getSelectionSize(
            antwika::character::PixelSelection{
                .fromCell = antwika::geometry::GridCell{2, 5},
                .toCell = antwika::geometry::GridCell{6, 3}}),
        (antwika::gfx::Size{.width = 5, .height = 3}));
}

TEST(CharacterMarksTest, SelectionSize_HoldsOnePixelForASelectionOfOne)
{
    const antwika::geometry::GridCell onlyCell{4, 4};

    EXPECT_EQ(
        antwika::character::getSelectionSize(
            antwika::character::PixelSelection{
                .fromCell = onlyCell,
                .toCell = onlyCell}),
        (antwika::gfx::Size{.width = 1, .height = 1}));
}

TEST(CharacterMarksTest, SelectionContains_TakesEveryPixelWithinTheSelection)
{
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{2, 3},
        .toCell = antwika::geometry::GridCell{4, 5}};

    for (std::uint32_t row = 3; row <= 5; ++row)
    {
        for (std::uint32_t column = 2; column <= 4; ++column)
        {
            EXPECT_TRUE(
                antwika::character::isSelectionContains(
                    selection,
                    antwika::geometry::GridCell{column, row}));
        }
    }
}

TEST(CharacterMarksTest, SelectionContains_TakesNoPixelBesideTheSelection)
{
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{2, 3},
        .toCell = antwika::geometry::GridCell{4, 5}};

    EXPECT_FALSE(
        antwika::character::isSelectionContains(
            selection, antwika::geometry::GridCell{1, 3}));
    EXPECT_FALSE(
        antwika::character::isSelectionContains(
            selection, antwika::geometry::GridCell{5, 5}));
    EXPECT_FALSE(
        antwika::character::isSelectionContains(
            selection, antwika::geometry::GridCell{3, 6}));
}

TEST(CharacterMarksTest, MovedSelection_KeepsTheSizeItWasGiven)
{
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{2, 3},
        .toCell = antwika::geometry::GridCell{5, 7}};
    const auto movedRect = antwika::character::getMovedSelection(selection, 3, -2);

    EXPECT_EQ(
        antwika::character::getSelectionSize(movedRect),
        antwika::character::getSelectionSize(selection));
    EXPECT_EQ(
        antwika::character::getSelectionOrigin(movedRect),
        (antwika::geometry::GridCell{5, 1}));
}

TEST(CharacterMarksTest, MovedSelection_HoldsASelectionWithinTheFrame)
{
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{0, 0},
        .toCell = antwika::geometry::GridCell{3, 3}};
    const auto clampedLow = antwika::character::getMovedSelection(selection, -4,
        -4);
    const auto clampedHigh = antwika::character::getMovedSelection(selection, 999,
        999);

    EXPECT_EQ(
        antwika::character::getSelectionOrigin(clampedLow),
        (antwika::geometry::GridCell{0, 0}));
    EXPECT_EQ(
        antwika::character::getSelectionOrigin(clampedHigh),
        (antwika::geometry::GridCell{
            kCharacterCellSize.width - 4, kCharacterCellSize.height - 4}));
    EXPECT_EQ(
        antwika::character::getSelectionSize(clampedHigh),
        antwika::character::getSelectionSize(selection));
}

TEST(CharacterMarksTest, CutFrom_TakesThePixelsItLeavesBlank)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{1, 1},
        .toCell = antwika::geometry::GridCell{2, 2}};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{1, 1}, kRedColor);

    const auto was = antwika::character::copiedFrom(sheet, 0, 0, selection);
    const auto cutBuffer = antwika::character::cutFrom(sheet, 0, 0, selection);

    EXPECT_EQ(cutBuffer, was);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 0, 0, antwika::geometry::GridCell{1, 1})
            .alpha,
        0);
}

TEST(CharacterMarksTest, CutFrom_LeavesTheFrameAsItStoodWhenLaidBack)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{3, 4},
        .toCell = antwika::geometry::GridCell{5, 6}};

    paintCharacter(
        sheet, 1, 1, antwika::geometry::GridCell{4, 5}, kRedColor);

    const auto was = sheet;
    const auto cutBuffer = antwika::character::cutFrom(sheet, 1, 1, selection);

    antwika::character::pasteInto(
        sheet, 1, 1, antwika::character::getSelectionOrigin(selection), cutBuffer);

    EXPECT_EQ(sheet.pixels, was.pixels);
}

TEST(CharacterMarksTest, MirroredHorizontally_ReadsEveryRowTheOtherWayAbout)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{0, 0},
        .toCell = antwika::geometry::GridCell{2, 1}};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{0, 1}, kRedColor);

    const auto mirroredBuffer = antwika::character::getMirroredHorizontally(
        antwika::character::copiedFrom(sheet, 0, 0, selection));

    EXPECT_EQ(mirroredBuffer.size.width, 3U);
    EXPECT_EQ(mirroredBuffer.size.height, 2U);
    EXPECT_EQ(mirroredBuffer.pixelColors.at(5), kRedColor);
    EXPECT_EQ(mirroredBuffer.pixelColors.at(3).alpha, 0);
}

TEST(
    CharacterMarksTest,
    MirroredHorizontally_GivesBackWhatItWasGivenTwiceOver)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{1, 1},
        .toCell = antwika::geometry::GridCell{5, 4}};

    for (const auto pixel :
         {antwika::geometry::GridCell{1, 1},
          antwika::geometry::GridCell{2, 3},
          antwika::geometry::GridCell{5, 4}})
    {
        paintCharacter(sheet, 1, 2, pixel, kRedColor);
    }

    const auto buffer = antwika::character::copiedFrom(sheet, 1, 2, selection);

    EXPECT_EQ(
        antwika::character::getMirroredHorizontally(
            antwika::character::getMirroredHorizontally(buffer)),
        buffer);
}

TEST(
    CharacterMarksTest,
    MirroredHorizontally_LeavesASelectionOfOneColumnAsItWas)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{3, 0},
        .toCell = antwika::geometry::GridCell{3, 2}};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{3, 1}, kRedColor);

    const auto buffer = antwika::character::copiedFrom(sheet, 0, 0, selection);

    EXPECT_EQ(antwika::character::getMirroredHorizontally(buffer), buffer);
}
