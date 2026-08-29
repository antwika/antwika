#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/render/CharacterSkins.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/tile/TilePaint.hpp>

#include "antwika/editor/fakes/FakeEditSteps.hpp"
#include "antwika/editor/fakes/FakeNotices.hpp"
#include "antwika/editor/fakes/ViewHarness.hpp"
#include "antwika/editor/ui/CharacterSheetView.hpp"
#include "antwika/editor/ui/CharacterView.hpp"

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
    antwika::render::CharacterSkins characterSkins;

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
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, characterSkins, {sheetOf(7), sheetOf(9)});

    EXPECT_EQ(characterSkins.getSheets().size(), 2U);
    EXPECT_EQ(sheets.getEditing(), 0U);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 7);
    EXPECT_NE(characterSkins.getPicture(1), nullptr);
    EXPECT_EQ(characterSkins.getPicture(2), nullptr);
}

TEST(CharacterSheetViewTest, TakeSkins_KeepsToTheCharactersWhereItShrank)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, characterSkins, {sheetOf(7), sheetOf(9)});
    sheets.switchTo(viewportRenderer, characterSkins, 1);
    sheets.takeSkins(viewportRenderer, characterSkins, {sheetOf(3)});

    EXPECT_EQ(sheets.getEditing(), 0U);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 3);
}

TEST(CharacterSheetViewTest, SwitchTo_PutsWhatWasDrawnBackOnItsSkin)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, characterSkins, {sheetOf(7), sheetOf(9)});
    sheets.getSheet().pixels.front() = 42;
    sheets.switchTo(viewportRenderer, characterSkins, 1);

    EXPECT_EQ(sheets.getEditing(), 1U);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 9);
    EXPECT_EQ(characterSkins.getSheets().at(0).pixels.front(), 42);
}

TEST(CharacterSheetViewTest, SwitchTo_LeavesTheBoardAloneWhereThereIsNoSuchSkin)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, characterSkins, {sheetOf(7)});
    sheets.switchTo(viewportRenderer, characterSkins, 4);

    EXPECT_EQ(sheets.getEditing(), 0U);
    EXPECT_EQ(sheets.getSheet().pixels.front(), 7);
}

TEST(CharacterSheetViewTest, EditFirst_GoesBackToTheFirstCharacter)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, characterSkins, {sheetOf(7), sheetOf(9)});
    sheets.switchTo(viewportRenderer, characterSkins, 1);
    sheets.editFirst();

    EXPECT_EQ(sheets.getEditing(), 0U);
}

TEST(CharacterSheetViewTest, SkinsAsDrawn_CarryTheUnkeptEditsOfTheBoard)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, characterSkins, {sheetOf(7), sheetOf(9)});
    sheets.getSheet().pixels.front() = 42;

    EXPECT_EQ(sheets.getSkinsAsDrawn(characterSkins).at(0).pixels.front(), 42);
    EXPECT_EQ(characterSkins.getSheets().at(0).pixels.front(), 7);

    sheets.keepEdits(viewportRenderer, characterSkins);

    EXPECT_EQ(characterSkins.getSheets().at(0).pixels.front(), 42);
}

TEST(CharacterSheetViewTest, Repaint_LaysASkinDownOverOneCharacter)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.takeSkins(viewportRenderer, characterSkins, {sheetOf(7), sheetOf(9)});
    sheets.repaint(viewportRenderer, characterSkins, 1, sheetOf(5));
    sheets.repaint(viewportRenderer, characterSkins, 9, sheetOf(6));

    EXPECT_EQ(characterSkins.getSheets().at(1).pixels.front(), 5);
    EXPECT_EQ(characterSkins.getSheets().size(), 2U);
}

TEST(CharacterSheetViewTest, Refresh_MakesThePictureAfreshOnlyOnceDrawnOn)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

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
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.getMark().selectedFrame = 3;

    EXPECT_CALL(innerRenderer, drawTexture)
        .Times(::testing::AtLeast(static_cast<int>(
            antwika::character::kCharacterWays
            * antwika::character::kCharacterFrames)));
    EXPECT_CALL(innerRenderer, drawRect).Times(::testing::AtLeast(1));
    EXPECT_CALL(innerRenderer, drawText).Times(1);

    sheets.drawSheet(
        viewportRenderer,
        antwika::editor::getCharacterSheetBounds(
            antwika::camera::kCanvasSize),
        antwika::editor::getCharacterDrawBounds(
            antwika::camera::kCanvasSize,
            antwika::editor::kRightPanelWidth));
}

TEST(CharacterSheetViewTest, Draw_LeavesTheBlownUpFrameOutWhereNoneIsMarked)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.getMark().selectedFrame.reset();

    EXPECT_CALL(innerRenderer, drawRect).Times(2);
    EXPECT_CALL(innerRenderer, drawText).Times(0);

    sheets.drawSheet(
        viewportRenderer,
        antwika::editor::getCharacterSheetBounds(
            antwika::camera::kCanvasSize),
        antwika::editor::getCharacterDrawBounds(
            antwika::camera::kCanvasSize,
            antwika::editor::kRightPanelWidth));
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
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(0));
    sheets.getMark().selection = getPatchSelection();
    sheets.getMark().selectedFrame = 0;

    sheets.commitFloatingPatch();

    EXPECT_FALSE(sheets.getMark().floatingPatchBuffer.has_value());
}

TEST(CharacterSheetViewTest, CommitFloatingPatch_LaysAHeldPatchDown)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;

    sheets.open(viewportRenderer, sheetOf(0));
    sheets.getMark().selectedFrame = 0;
    sheets.getMark().selection = getPatchSelection();
    sheets.getMark().floatingPatchBuffer = antwika::character::copiedFrom(
        sheets.getSheet(), 0, 0, getPatchSelection());

    sheets.commitFloatingPatch();

    EXPECT_FALSE(sheets.getMark().floatingPatchBuffer.has_value());
}

TEST(CharacterSheetViewTest, MirrorSelection_MarksAStepWhenItPaintsTheSheet)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;
    FakeEditSteps steps;

    sheets.open(viewportRenderer, sheetOf(0));
    sheets.getMark().selectedFrame = 0;
    sheets.getMark().selection = getPatchSelection();

    sheets.mirrorSelection(steps);

    EXPECT_EQ(steps.steps, 1U);
}

TEST(CharacterSheetViewTest, MirrorSelection_MarksNoStepForAHeldPatch)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;
    FakeEditSteps steps;

    sheets.open(viewportRenderer, sheetOf(0));
    sheets.getMark().selectedFrame = 0;
    sheets.getMark().selection = getPatchSelection();
    sheets.getMark().floatingPatchBuffer = antwika::character::copiedFrom(
        sheets.getSheet(), 0, 0, getPatchSelection());

    sheets.mirrorSelection(steps);

    EXPECT_EQ(steps.steps, 0U);
    EXPECT_TRUE(sheets.getMark().floatingPatchBuffer.has_value());
}

TEST(CharacterSheetViewTest, MirrorSelection_MarksNoStepWithNothingSelected)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;
    antwika::render::CharacterSkins characterSkins;
    FakeEditSteps steps;

    sheets.open(viewportRenderer, sheetOf(0));

    sheets.mirrorSelection(steps);

    EXPECT_EQ(steps.steps, 0U);
}

TEST(CharacterSheetViewTest, Claims_NamesTheCharacterTab)
{
    const CharacterSheetView sheets;

    EXPECT_TRUE(sheets.claims(antwika::editor::View::Character, false));
    EXPECT_FALSE(sheets.claims(antwika::editor::View::Character, true));
}

namespace
{
    class CharacterSheetStrokeTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] static antwika::input::Position getPositionOver(
            const antwika::geometry::GridCell pixelCell)
        {
            const auto place = antwika::character::getCharacterPixelPlace(
                antwika::editor::getCharacterCanvasRect(
                    antwika::editor::getCharacterDrawBounds(
                        antwika::camera::kCanvasSize,
                        antwika::editor::kRightPanelWidth)),
                pixelCell);

            return antwika::input::Position{
                .x = static_cast<std::int32_t>(
                    place.originPoint.x + (place.size.width / 2.0F)),
                .y = static_cast<std::int32_t>(
                    place.originPoint.y + (place.size.height / 2.0F))};
        }

        [[nodiscard]] antwika::gfx::Color colorAt(
            const antwika::geometry::GridCell pixelCell) const
        {
            return antwika::character::getCharacterPixelColor(
                harness.characterView.getSheet(), 0, 0, pixelCell);
        }

        void strokeAcross(
            const antwika::geometry::GridCell fromCell,
            const antwika::geometry::GridCell toCell)
        {
            const auto viewContext = harness.contextNow();

            ASSERT_TRUE(
                harness.characterView.consumePress(
                    viewContext,
                    antwika::input::PointerButtonPressed{
                        .button = antwika::input::MouseButton::Left,
                        .position = getPositionOver(fromCell)}));
            ASSERT_EQ(colorAt(fromCell).alpha, 0U);
            ASSERT_EQ(steps.pushCount, 0U);
            ASSERT_TRUE(
                harness.characterView.consumeRelease(
                    viewContext,
                    antwika::input::PointerButtonReleased{
                        .button = antwika::input::MouseButton::Left,
                        .position = getPositionOver(toCell)}));
        }

        NiceMock<antwika::log::mocks::MockLogger> logger;
        antwika::editor::fakes::FakeEditSteps steps;
        antwika::editor::fakes::FakeNotices notices;
        antwika::editor::fakes::ViewHarness harness{logger, steps, notices};
    };
}

TEST_F(CharacterSheetStrokeTest, LineStroke_LaysTheWholeLineDownAtRelease)
{
    const antwika::geometry::GridCell fromCell{.column = 2, .row = 3};
    const antwika::geometry::GridCell toCell{.column = 11, .row = 7};

    harness.preferences.paint = antwika::editor::Paint::Line;
    harness.inkPicker.activeInk = 2;

    strokeAcross(fromCell, toCell);

    EXPECT_EQ(steps.pushCount, 1U);

    const auto inkColor = harness.document.map.paletteColors.at(2);

    for (const auto cell : antwika::tile::getLinePixels(fromCell, toCell))
    {
        EXPECT_EQ(colorAt(cell), inkColor);
    }

    EXPECT_EQ(
        colorAt(antwika::geometry::GridCell{.column = 2, .row = 7}).alpha, 0U);
}

TEST_F(CharacterSheetStrokeTest, RectStroke_LaysTheOutlineDownAtRelease)
{
    const antwika::geometry::GridCell fromCell{.column = 3, .row = 4};
    const antwika::geometry::GridCell toCell{.column = 10, .row = 9};

    harness.preferences.paint = antwika::editor::Paint::Rect;
    harness.inkPicker.activeInk = 1;

    strokeAcross(fromCell, toCell);

    EXPECT_EQ(steps.pushCount, 1U);

    const auto inkColor = harness.document.map.paletteColors.at(1);

    for (const auto cell : antwika::tile::getRectPixels(fromCell, toCell))
    {
        EXPECT_EQ(colorAt(cell), inkColor);
    }

    EXPECT_EQ(
        colorAt(antwika::geometry::GridCell{.column = 6, .row = 6}).alpha, 0U);
}


TEST(CharacterSheetViewTest, Draw_ClipsTheSheetAndTheDrawingToTheirPanels)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    CharacterSheetView sheets;

    sheets.open(viewportRenderer, sheetOf(1));
    sheets.getMark().selectedFrame = 0;

    EXPECT_CALL(innerRenderer, beginClip).Times(2);
    EXPECT_CALL(innerRenderer, endClip).Times(2);

    sheets.drawSheet(
        viewportRenderer,
        antwika::editor::getCharacterSheetBounds(
            antwika::camera::kCanvasSize),
        antwika::editor::getCharacterDrawBounds(
            antwika::camera::kCanvasSize,
            antwika::editor::kRightPanelWidth));
}
