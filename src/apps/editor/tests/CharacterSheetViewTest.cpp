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
#include <antwika/render/CharacterSkins.hpp>
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
        const auto size = antwika::character::getCharacterSheetSize();

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
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));

    EXPECT_NE(sheets.getTexture(), nullptr);
    EXPECT_NE(sheets.getChecker(), nullptr);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 1);
}

TEST(CharacterSheetViewTest, TakeSkins_PutsTheOneBeingEditedOnTheBoard)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, rosterSkins, {sheetOf(7), sheetOf(9)});

    EXPECT_EQ(rosterSkins.getSheets().size(), 2U);
    EXPECT_EQ(sheets.getEditing(), 0U);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 7);
    EXPECT_NE(rosterSkins.getPicture(1), nullptr);
    EXPECT_EQ(rosterSkins.getPicture(2), nullptr);
}

TEST(CharacterSheetViewTest, TakeSkins_KeepsToTheRosterWhereItShrank)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, rosterSkins, {sheetOf(7), sheetOf(9)});
    sheets.switchTo(viewportRenderer, rosterSkins, 1);
    sheets.takeSkins(viewportRenderer, rosterSkins, {sheetOf(3)});

    EXPECT_EQ(sheets.getEditing(), 0U);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 3);
}

TEST(CharacterSheetViewTest, SwitchTo_PutsWhatWasDrawnBackOnItsSkin)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, rosterSkins, {sheetOf(7), sheetOf(9)});
    sheets.getSheet().pixels.front() = 42;
    sheets.switchTo(viewportRenderer, rosterSkins, 1);

    EXPECT_EQ(sheets.getEditing(), 1U);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 9);
    EXPECT_EQ(rosterSkins.getSheets().at(0).pixels.front(), 42);
}

TEST(CharacterSheetViewTest, SwitchTo_LeavesTheBoardAloneWhereThereIsNoSuchSkin)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, rosterSkins, {sheetOf(7)});
    sheets.switchTo(viewportRenderer, rosterSkins, 4);

    EXPECT_EQ(sheets.getEditing(), 0U);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 7);
}

TEST(CharacterSheetViewTest, EditFirst_GoesBackToTheHeadOfTheRoster)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, rosterSkins, {sheetOf(7), sheetOf(9)});
    sheets.switchTo(viewportRenderer, rosterSkins, 1);
    sheets.editFirst();

    EXPECT_EQ(sheets.getEditing(), 0U);
}

TEST(CharacterSheetViewTest, SkinsAsDrawn_CarryTheUnkeptEditsOfTheBoard)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, rosterSkins, {sheetOf(7), sheetOf(9)});
    sheets.getSheet().pixels.front() = 42;

    EXPECT_EQ(sheets.getSkinsAsDrawn(rosterSkins).at(0).pixels.front(), 42);
    EXPECT_EQ(rosterSkins.getSheets().at(0).pixels.front(), 7);

    sheets.keepEdits(viewportRenderer, rosterSkins);

    EXPECT_EQ(rosterSkins.getSheets().at(0).pixels.front(), 42);
}

TEST(CharacterSheetViewTest, Repaint_LaysASkinDownOverOneOfTheRoster)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, rosterSkins, {sheetOf(7), sheetOf(9)});
    sheets.repaint(viewportRenderer, rosterSkins, 1, sheetOf(5));
    sheets.repaint(viewportRenderer, rosterSkins, 9, sheetOf(6));

    EXPECT_EQ(rosterSkins.getSheets().at(1).pixels.front(), 5);
    EXPECT_EQ(rosterSkins.getSheets().size(), 2U);
}

TEST(CharacterSheetViewTest, Refresh_MakesThePictureAfreshOnlyOnceDrawnOn)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

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
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.mark.selectedFrame = 3;

    EXPECT_CALL(innerRenderer, drawTexture)
        .Times(::testing::AtLeast(static_cast<int>(
            antwika::character::kCharacterWays
            * antwika::character::kCharacterFrames)));
    EXPECT_CALL(innerRenderer, drawRect).Times(::testing::AtLeast(1));
    EXPECT_CALL(innerRenderer, drawText).Times(1);

    sheets.drawSheet(viewportRenderer);
}

TEST(CharacterSheetViewTest, Draw_LeavesTheBlownUpFrameOutWhereNoneIsMarked)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.mark.selectedFrame.reset();

    EXPECT_CALL(innerRenderer, drawRect).Times(0);
    EXPECT_CALL(innerRenderer, drawText).Times(0);

    sheets.drawSheet(viewportRenderer);
}

namespace
{
    class FakeEditSteps final : public antwika::editor::IEditSteps
    {
    public:
        void pushUndo() override
        {
            ++steps;
        }

        std::size_t steps = 0;
    };

    [[nodiscard]] antwika::character::PixelSelection getPatchSelection()
    {
        return antwika::character::PixelSelection{
            .fromCell = antwika::geometry::GridCell{.column = 0, .row = 0},
            .toCell = antwika::geometry::GridCell{.column = 2, .row = 2}};
    }
}

TEST(CharacterSheetViewTest, CommitFloatingPatch_LeavesTheSheetWithNoPatchHeld)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(0));
    sheets.mark.selection = getPatchSelection();
    sheets.mark.selectedFrame = 0;

    sheets.commitFloatingPatch();

    EXPECT_FALSE(sheets.mark.floatingPatchBuffer.has_value());
}

TEST(CharacterSheetViewTest, CommitFloatingPatch_LaysAHeldPatchDown)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;

    sheets.open(viewportRenderer, sheetOf(0));
    sheets.mark.selectedFrame = 0;
    sheets.mark.selection = getPatchSelection();
    sheets.mark.floatingPatchBuffer = antwika::character::copiedFrom(
        sheets.getSheet(), 0, 0, getPatchSelection());

    sheets.commitFloatingPatch();

    EXPECT_FALSE(sheets.mark.floatingPatchBuffer.has_value());
}

TEST(CharacterSheetViewTest, MirrorSelection_MarksAStepWhenItPaintsTheSheet)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;
    FakeEditSteps steps;

    sheets.open(viewportRenderer, sheetOf(0));
    sheets.mark.selectedFrame = 0;
    sheets.mark.selection = getPatchSelection();

    sheets.mirrorSelection(steps);

    EXPECT_EQ(steps.steps, 1U);
}

TEST(CharacterSheetViewTest, MirrorSelection_MarksNoStepForAHeldPatch)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;
    FakeEditSteps steps;

    sheets.open(viewportRenderer, sheetOf(0));
    sheets.mark.selectedFrame = 0;
    sheets.mark.selection = getPatchSelection();
    sheets.mark.floatingPatchBuffer = antwika::character::copiedFrom(
        sheets.getSheet(), 0, 0, getPatchSelection());

    sheets.mirrorSelection(steps);

    EXPECT_EQ(steps.steps, 0U);
    EXPECT_TRUE(sheets.mark.floatingPatchBuffer.has_value());
}

TEST(CharacterSheetViewTest, MirrorSelection_MarksNoStepWithNothingSelected)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins rosterSkins;
    FakeEditSteps steps;

    sheets.open(viewportRenderer, sheetOf(0));

    sheets.mirrorSelection(steps);

    EXPECT_EQ(steps.steps, 0U);
}

TEST(CharacterSheetViewTest, Claims_NamesTheCharacterTab)
{
    const CharacterSheetView sheets;

    EXPECT_TRUE(sheets.claims(antwika::map::View::Character, false));
    EXPECT_FALSE(sheets.claims(antwika::map::View::Character, true));
}

