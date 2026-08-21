#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/character/Character.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

#include "antwika/editor/ui/CharacterSheetView.hpp"

using antwika::editor::CharacterSheetView;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::ITexture;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using ::testing::NiceMock;

namespace
{
    constexpr Size kWindowSize{.width = 960, .height = 540};

    constexpr Size kCanvasSize{.width = 480, .height = 270};

    [[nodiscard]] Bitmap sheetOf(const std::uint8_t shade)
    {
        const auto size = antwika::character::characterSheetSize();

        return Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width) * size.height
                    * antwika::gfx::kBytesPerPixel,
                shade)};
    }

    void handsOutTextures(NiceMock<MockRenderer> &innerRenderer)
    {
        ON_CALL(innerRenderer, createTexture(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const Bitmap &bitmap)
                {
                    return std::unique_ptr<ITexture>{
                        std::make_unique<NiceMock<MockTexture>>()};
                });
    }
}

TEST(CharacterSheetViewTest, Open_CarriesTheSheetAndItsBackingToPictures)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));

    EXPECT_NE(sheets.texture(), nullptr);
    EXPECT_NE(sheets.checker(), nullptr);
    EXPECT_EQ(sheets.sheet().pixels.front(), 1);
}

TEST(CharacterSheetViewTest, TakeSkins_PutsTheOneBeingEditedOnTheBoard)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, {sheetOf(7), sheetOf(9)});

    EXPECT_EQ(sheets.skins().size(), 2U);
    EXPECT_EQ(sheets.editing(), 0U);
    EXPECT_EQ(sheets.sheet().pixels.front(), 7);
    EXPECT_NE(sheets.skinTexture(1), nullptr);
    EXPECT_EQ(sheets.skinTexture(2), nullptr);
}

TEST(CharacterSheetViewTest, TakeSkins_KeepsToTheRosterWhereItShrank)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, {sheetOf(7), sheetOf(9)});
    sheets.switchTo(viewportRenderer, 1);
    sheets.takeSkins(viewportRenderer, {sheetOf(3)});

    EXPECT_EQ(sheets.editing(), 0U);
    EXPECT_EQ(sheets.sheet().pixels.front(), 3);
}

TEST(CharacterSheetViewTest, SwitchTo_PutsWhatWasDrawnBackOnItsSkin)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, {sheetOf(7), sheetOf(9)});
    sheets.sheet().pixels.front() = 42;
    sheets.switchTo(viewportRenderer, 1);

    EXPECT_EQ(sheets.editing(), 1U);
    EXPECT_EQ(sheets.sheet().pixels.front(), 9);
    EXPECT_EQ(sheets.skins().at(0).pixels.front(), 42);
}

TEST(CharacterSheetViewTest, SwitchTo_LeavesTheBoardAloneWhereThereIsNoSuchSkin)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, {sheetOf(7)});
    sheets.switchTo(viewportRenderer, 4);

    EXPECT_EQ(sheets.editing(), 0U);
    EXPECT_EQ(sheets.sheet().pixels.front(), 7);
}

TEST(CharacterSheetViewTest, EditFirst_GoesBackToTheHeadOfTheRoster)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, {sheetOf(7), sheetOf(9)});
    sheets.switchTo(viewportRenderer, 1);
    sheets.editFirst();

    EXPECT_EQ(sheets.editing(), 0U);
}

TEST(CharacterSheetViewTest, SkinsAsDrawn_CarryTheUnkeptEditsOfTheBoard)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, {sheetOf(7), sheetOf(9)});
    sheets.sheet().pixels.front() = 42;

    EXPECT_EQ(sheets.skinsAsDrawn().at(0).pixels.front(), 42);
    EXPECT_EQ(sheets.skins().at(0).pixels.front(), 7);

    sheets.keepEdits(viewportRenderer);

    EXPECT_EQ(sheets.skins().at(0).pixels.front(), 42);
}

TEST(CharacterSheetViewTest, Repaint_LaysASkinDownOverOneOfTheRoster)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, {sheetOf(7), sheetOf(9)});
    sheets.repaint(viewportRenderer, 1, sheetOf(5));
    sheets.repaint(viewportRenderer, 9, sheetOf(6));

    EXPECT_EQ(sheets.skins().at(1).pixels.front(), 5);
    EXPECT_EQ(sheets.skins().size(), 2U);
}

TEST(CharacterSheetViewTest, Refresh_MakesThePictureAfreshOnlyOnceDrawnOn)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));

    EXPECT_CALL(innerRenderer, createTexture).Times(0);

    sheets.refresh(viewportRenderer);

    ::testing::Mock::VerifyAndClearExpectations(&innerRenderer);
    handsOutTextures(innerRenderer);

    EXPECT_CALL(innerRenderer, createTexture).Times(1);

    sheets.touch();
    sheets.refresh(viewportRenderer);
}

TEST(CharacterSheetViewTest, Draw_DrawsEveryFrameAndTheOneMarkedOut)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.mark.selectedFrame = 3;

    EXPECT_CALL(innerRenderer, drawTexture)
        .Times(::testing::AtLeast(static_cast<int>(
            antwika::character::kCharacterWays
            * antwika::character::kCharacterFrames)));
    EXPECT_CALL(innerRenderer, drawRect).Times(::testing::AtLeast(1));
    EXPECT_CALL(innerRenderer, drawText).Times(1);

    sheets.draw(viewportRenderer);
}

TEST(CharacterSheetViewTest, Draw_LeavesTheBlownUpFrameOutWhereNoneIsMarked)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.mark.selectedFrame.reset();

    EXPECT_CALL(innerRenderer, drawRect).Times(0);
    EXPECT_CALL(innerRenderer, drawText).Times(0);

    sheets.draw(viewportRenderer);
}
